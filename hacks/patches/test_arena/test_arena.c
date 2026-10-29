#include "patches/test_arena/test_arena.h"
#include "actor/enemySpawner.h"
#include "game.h"
#include "gba_io.h"
#include "lib/mission_scripts.h"
#include "lib/movement.h"
#include "lib/profile.h"
#include "lib/summon_registry.h"
#include "lib/timed_effect.h"
#include "memory.h"
#include "mission.h"
#include "patches/debug_cheats/debug_cheats.h"
#include "patches/debug_cheats/debug_lab.h"
#include "patches/mod_ram/memory_diagnostics.h"
#include "patches/legacy_skills/legacy_skills.h"
#include "patches/skill_expansion/skill_delivery.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "patches/skill_expansion/skill_expansion_timeline.h"
#include "rng.h"
#include "save.h"
#include "scene.h"
#include "scene/gameplay.h"
#include "sprite.h"
#include "stats.h"
#include "text.h"
#include "variables.h"
#if HACK_fast_waitstates
#include "patches/fast_waitstates/fast_waitstates.h"
#endif
#if HACK_experimental
#include "patches/experimental/experimental.h"
#endif

#if !HACK_debug_cheats || !HACK_skill_expansion
#error "test_arena requires debug_cheats and skill_expansion"
#endif
#ifndef TEST_ARENA_HERO
#define TEST_ARENA_HERO HERO_ID_ARAGORN
#endif
_Static_assert(TEST_ARENA_HERO < SKILL_EXPANSION_HERO_COUNT, "invalid arena hero");

// mGBA debug log registers.
#define REG_DEBUG_STRING ((volatile char *)0x04fff600)
#define REG_DEBUG_FLAGS (*(volatile u16 *)0x04fff700)
#define REG_DEBUG_ENABLE (*(volatile u16 *)0x04fff780)
#define DEBUG_STRING_SIZE 0x100
#define DEBUG_SEND_WARN 0x102

#define SWEEP_SETTLE_TICKS 10
#define BENCH_SPOTS 8
// Foes the benchmark spawns per tick at most.
#define BENCH_SPAWNS_PER_TICK 2
#define BENCH_WARMUP_TICKS 60

// CPU sampler: Timer 2 at 16.78 MHz / 1024, reloading every 125 counts (about 131 Hz).
#define SAMPLE_CAPACITY 64
#define SAMPLE_TIMER_RELOAD (0x10000 - 125)
#define SAMPLE_TIMER_START 0xc3
#define SAMPLE_IRQ_TIMER2 (1 << 5)
#define BIOS_ISR_POINTER (*(void *volatile *)0x03007ffc)

typedef struct SampleBuffer
{
    // Samples waiting to be printed.
    u32 count;
    // Samples lost because the buffer was full.
    u32 dropped;
    // The game's interrupt handler, run for every other interrupt.
    void *chain;
    // Interrupted program counters.
    u32 pcs[SAMPLE_CAPACITY];
} SampleBuffer;

SampleBuffer *testArena_samples;

// Runs in IRQ mode from the BIOS vector. The BIOS frame at sp holds r0-r3, r12 and lr_irq,
// and lr_irq is the interrupted instruction plus 4.
__attribute__((naked, target("arm"))) static void sampleIsr(void)
{
    asm volatile("ldr r0, =0x04000200\n"
                 "ldrh r1, [r0, #2]\n"
                 "tst r1, #0x20\n"
                 "beq 2f\n"
                 "mov r1, #0x20\n"
                 "strh r1, [r0, #2]\n"
                 "ldr r2, =testArena_samples\n"
                 "ldr r2, [r2]\n"
                 "ldr r3, [r2]\n"
                 "cmp r3, #64\n"
                 "ldrhs r1, [r2, #4]\n"
                 "addhs r1, r1, #1\n"
                 "strhs r1, [r2, #4]\n"
                 "bhs 1f\n"
                 "ldr r12, [sp, #20]\n"
                 "sub r12, r12, #4\n"
                 "add r1, r2, #12\n"
                 "str r12, [r1, r3, lsl #2]\n"
                 "add r3, r3, #1\n"
                 "str r3, [r2]\n"
                 "1:\n"
                 "ldr r1, [r0]\n"
                 "and r1, r1, r1, lsr #16\n"
                 "movs r1, r1, lsl #18\n"
                 "bxeq lr\n"
                 "2:\n"
                 "ldr r2, =testArena_samples\n"
                 "ldr r2, [r2]\n"
                 "ldr r2, [r2, #8]\n"
                 "mov r0, #0x04000000\n"
                 "bx r2\n"
                 ".ltorg\n");
}
_Static_assert(__builtin_offsetof(SampleBuffer, pcs) == 12, "sampleIsr layout");
_Static_assert(SAMPLE_CAPACITY == 64, "sampleIsr capacity");

const volatile TestArenaConfig testArena_config = {.hero = TEST_ARENA_HERO};
volatile TestArenaControl *g_TestArena;
static Vector2Fp16 anchor;

// Each sweep row casts from a fresh fixture, clears while effects may be live, then repeats.
static const u8 SweepPlan[] = {
    TEST_ARENA_RESET, TEST_ARENA_CAST, TEST_ARENA_WAIT, TEST_ARENA_CLEAR, TEST_ARENA_WAIT,
    TEST_ARENA_RESET, TEST_ARENA_CAST, TEST_ARENA_WAIT, TEST_ARENA_CLEAR, TEST_ARENA_WAIT,
};

void testArena_reseed(void)
{
    g_RngSeed = 0x524f544b;
    g_RngReseedCount = 0;
    g_RngGameplayRemaining = g_RngMtRemaining = -1;
    rng_seedState(g_RngSeed);
    g_RngGameplayLastByte = 0;
    g_Unknown030054f4 = 0;
}

void testArena_boot(void)
{
    u32 hero = testArena_config.hero;

    REG_DEBUG_ENABLE = 0xc0de;
    g_TestArena = hero < SKILL_EXPANSION_HERO_COUNT ? memory_zalloc(sizeof(*g_TestArena)) : NULL;
    if (g_TestArena == NULL)
    {
        game_requestSceneChange(SCENE_ID_MAIN_MENU);
        return;
    }
    memory_memcpy(&g_SaveGlobals, SaveHeaderTemplate, sizeof(g_SaveGlobals));
    g_SaveGlobals.d.language = LANGUAGE_EN;
    g_SaveGlobals.d.languageSet = 1;
    text_setLanguage(LANGUAGE_EN);
    memory_memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    g_PlayerGlobals.activePlayerCount = 1;
    PLAYER(0).heroId = hero;
    for (u32 i = 0; i < ARRAY_COUNT(PLAYER(0).inventory.array); i++)
    {
        item_clearSlot(&PLAYER(0).inventory.array[i].word);
    }
    player_initHeroStats(hero, 0);
    player_initHeroLoadout(hero, 0);
    player_initItemsAndHollow(0);
    player_applyOptionFlags(0);
    for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        if (PLAYER_ACTIVE_SKILL(0, row).callback != NULL)
        {
            *skillExpansion_activePurchased(0, row) = 1;
            *skillExpansion_activeLevel(0, row) = 1;
        }
    }
    g_DebugCheatsState = (struct DebugCheatsState){.tag = DEBUG_CHEATS_TAG};
    g_GameFlags.p = GAME_FLAG_BONUS_MAP_ACTIVE;
    hackLib_suppressMissionScripts = TRUE;
    g_MissionVariant = 0;
    g_MissionSpawnPoint[0] = g_MissionSpawnPoint[1] = 0;
    *g_TestArena = (TestArenaControl){.magic = 0x41524e41, .version = 3, .level = 1};
#if HACK_fast_waitstates
    fastWaitstates_apply();
#endif
#if HACK_experimental
    experimental_useDefaults();
#endif
    game_requestSceneChangeWithArgs(SCENE_ID_GAMEPLAY, GAMEPLAY_MODE_START_HEALED,
                                    MISSION_ISENGARD_CAMPSITE);
}

static void refill(void)
{
    PLAYER(0).currentHp = PLAYER_STAT(0, STAT_MAX_HP);
    PLAYER(0).currentSpirit = PLAYER_STAT(0, STAT_MAX_SPIRIT);
}

static bool findAnchor(Actor *owner)
{
    Vector2Fp16 origin = owner->coords;
    for (s32 radius = 0; radius <= 160; radius += 16)
    {
        for (s32 y = -radius; y <= radius; y += 16)
        {
            for (s32 x = -radius; x <= radius; x += 16)
            {
                if (ABS(x) != radius && ABS(y) != radius)
                {
                    continue;
                }
                bool clear = TRUE;
                for (s32 dy = -16; dy <= 16 && clear; dy += 16)
                {
                    for (s32 dx = -16; dx <= 96; dx += 16)
                    {
                        Vector2Fp16 at = {origin.x + (x + dx) * FP16_16_ONE,
                                          origin.y + (y + dy) * FP16_16_ONE};
                        if (!hackLib_canLandAt(owner, &at))
                        {
                            clear = FALSE;
                            break;
                        }
                    }
                }
                if (clear)
                {
                    anchor = (Vector2Fp16){origin.x + x * FP16_16_ONE, origin.y + y * FP16_16_ONE};
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static bool reset(u32 encounter)
{
    if (encounter > 2)
    {
        return FALSE;
    }
    Actor *owner = PLAYER(0).ownerActor;
    skillDelivery_clear(0);
    skillExpansion_clearTimeline(0);
    hackLib_clearOwnerEffects(owner);
    hackLib_clearOwnerSummons(owner);
    debugLab_clearTargets();
    // Hero systems restart the way skillExpansion_runtimeBegin restarts them for a live hero.
    const SkillExpansionHeroKit *kit = skillExpansion_runtime[0].kit;
    if (kit != NULL && kit->cleanup != NULL)
    {
        kit->cleanup(0, owner);
    }
    skillExpansion_eliteCleanup(0, owner);
    skillExpansion_eliteReset(0, FALSE);
    if (kit != NULL && kit->reset != NULL)
    {
        kit->reset(0);
    }
    sprite_setPosition(owner, anchor.x, anchor.y);
    owner->velocity = (Vector2Fp16){0};
    owner->facing = 2;
    skillExpansion_runtime[0].pending = FALSE;
    skillExpansion_runtime[0].paidSpirit = 0;
    memory_memset(skillExpansion_runtime[0].cooldowns, 0,
                  sizeof(skillExpansion_runtime[0].cooldowns));
    refill();
    testArena_reseed();
    debugLab_resetCombatStats();
    return debugLab_prepareEncounter(0, encounter) == (encounter == 1 ? 3u : 1u);
}

static void debugPrint(const char *text)
{
    u32 i = 0;
    for (; text[i] != 0 && i < DEBUG_STRING_SIZE - 1; i++)
    {
        REG_DEBUG_STRING[i] = text[i];
    }
    REG_DEBUG_STRING[i] = 0;
    REG_DEBUG_FLAGS = DEBUG_SEND_WARN;
}

static char *appendText(char *output, const char *text)
{
    while (*text)
    {
        *output++ = *text++;
    }
    *output = 0;
    return output;
}

static char *appendNumber(char *output, s32 value)
{
    char digits[10];
    u32 magnitude = value < 0 ? -(u32)value : (u32)value;
    u32 count = 0;

    *output++ = ' ';
    if (value < 0)
    {
        *output++ = '-';
    }
    do
    {
        digits[count++] = '0' + magnitude % 10;
        magnitude /= 10;
    } while (magnitude != 0);
    while (count > 0)
    {
        *output++ = digits[--count];
    }
    *output = 0;
    return output;
}

static void sweepPrintRow(void)
{
    volatile TestArenaControl *arena = g_TestArena;
    volatile u32 *values = arena->sweepValues;
    ModRamMemoryStats memory;
    char line[DEBUG_STRING_SIZE];
    char *output = appendText(line, "ARENA_ROW");

    modRam_getMemoryStats(&memory);
    values[TEST_ARENA_SWEEP_ROW] = arena->sweepRow;
    values[TEST_ARENA_SWEEP_LAG_TICKS] = arena->lagTicks - values[TEST_ARENA_SWEEP_LAG_TICKS];
    values[TEST_ARENA_SWEEP_WORST_TICK_VBLANKS] = arena->worstTickVblanks;
    values[TEST_ARENA_SWEEP_HEAP_ROOM] = memory.heapEnd - memory.peakTop;
    values[TEST_ARENA_SWEEP_ALLOC_FAILURES] = memory.failures;
    values[TEST_ARENA_SWEEP_INVALID_FREES] = memory.invalidFrees;
    values[TEST_ARENA_SWEEP_BSS_GUARD_INTACT] = modRam_isBssGuardIntact();
    values[TEST_ARENA_SWEEP_HERO_STATE_MISUSE] = skillExpansion_heroStateMisuse;
    values[TEST_ARENA_SWEEP_EFFECTS_AFTER_CLEAR] = hackLib_effectPoolUsed();
    for (u32 i = 0; i < TEST_ARENA_SWEEP_VALUE_COUNT; i++)
    {
        output = appendNumber(output, values[i]);
    }
    debugPrint(line);
}

/** Print a slow tick with the sweep position, ticks since the arena reset and the actor count. */
static void sweepPrintLag(u32 vblanks)
{
    volatile TestArenaControl *arena = g_TestArena;
    char line[DEBUG_STRING_SIZE];
    char *output = appendText(line, "ARENA_LAG");
    u32 actors = 0;

    for (Actor *cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        actors++;
    }
    output = appendNumber(output, arena->sweepRow);
    output = appendNumber(output, arena->sweepStep);
    output = appendNumber(output, vblanks);
    output = appendNumber(output, arena->ticks - arena->resetTick);
    appendNumber(output, actors);
    debugPrint(line);
}

static void sweepFinishStep(u32 step)
{
    volatile TestArenaControl *arena = g_TestArena;
    volatile u32 *values = arena->sweepValues;

    switch (step)
    {
    case 0:
        values[TEST_ARENA_SWEEP_PLAYER_DOWNED] = 0;
        values[TEST_ARENA_SWEEP_LAG_TICKS] = arena->lagTicks;
        arena->worstTickVblanks = 0;
        break;
    case 1:
        values[TEST_ARENA_SWEEP_ACCEPTED] = arena->result;
        break;
    case 2:
        values[TEST_ARENA_SWEEP_HITS] = arena->hits;
        values[TEST_ARENA_SWEEP_KILLS] = arena->kills;
        values[TEST_ARENA_SWEEP_DAMAGE] = arena->damage;
        values[TEST_ARENA_SWEEP_PLAYER_HP] = arena->playerHp;
        break;
    case 6:
        values[TEST_ARENA_SWEEP_RECAST] = arena->result;
        break;
    }
}

/** One past the last active row the arena casts: the hero's Two Towers rows too when loaded. */
static u32 castRowEnd(void)
{
#if PC_EDITION
    if (legacySkills_loaded(0))
    {
        return SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + legacySkills_activeCount();
    }
#endif
    return SKILL_EXPANSION_ACTIVE_ROW_COUNT;
}

/** Issue the next automatic sweep step once the previous command is acknowledged. */
static void sweepAdvance(void)
{
    volatile TestArenaControl *arena = g_TestArena;

    if (arena->sweepRow < testArena_config.sweepFirstRow)
    {
        arena->sweepRow = testArena_config.sweepFirstRow;
    }
    if (testArena_config.sweepTicks == 0 || arena->sweepRow >= castRowEnd() ||
        arena->request != arena->acknowledged)
    {
        return;
    }
    if (arena->sweepStep > 0)
    {
        sweepFinishStep(arena->sweepStep - 1);
    }
    if (arena->sweepStep == ARRAY_COUNT(SweepPlan))
    {
        sweepPrintRow();
        arena->sweepStep = 0;
        if (++arena->sweepRow == castRowEnd())
        {
            char line[DEBUG_STRING_SIZE];
            char *output = appendText(line, "ARENA_DONE");
            output = appendNumber(output, arena->ticks);
            output = appendNumber(output, arena->lagTicks);
            appendNumber(output, arena->heapRoom);
            debugPrint(line);
            return;
        }
    }
    u32 step = arena->sweepStep++;
    u32 command = SweepPlan[step];
    u32 argument = 0;
    if (command == TEST_ARENA_RESET)
    {
        argument = testArena_config.sweepFixture;
    }
    else if (command == TEST_ARENA_CAST)
    {
        argument = arena->sweepRow;
    }
    else if (command == TEST_ARENA_WAIT)
    {
        argument = SweepPlan[step - 1] == TEST_ARENA_CLEAR ? SWEEP_SETTLE_TICKS
                                                           : testArena_config.sweepTicks;
    }
    arena->command = command;
    arena->argument = argument;
    arena->level = 1;
    arena->cosmetic = 0;
    arena->request++;
}

static u32 countFoes(void)
{
    u32 count = 0;

    for (Actor *cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        count += skillExpansion_isFoe(cursor) && !(cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
    }
    return count;
}

/** Spawn an orc archer on the next spot of growing rings around the player. */
static void benchSpawnFoe(void)
{
    static const s8 Spots[BENCH_SPOTS][2] = {
        {48, 0}, {-48, 0}, {0, 40}, {0, -40}, {40, 32}, {-40, 32}, {40, -32}, {-40, -32},
    };
    Actor *owner = PLAYER(0).ownerActor;
    u32 index = g_TestArena->benchSpawned++;
    u32 spot = index % BENCH_SPOTS;
    s32 scale = 2 + index / BENCH_SPOTS % 3;
    Actor spawner = {0};
    Vector2Fp16 preferred = {owner->coords.x + Spots[spot][0] * scale / 2 * FP16_16_ONE,
                             owner->coords.y + Spots[spot][1] * scale / 2 * FP16_16_ONE};
    Vector2Fp16 landing;

    spawner.coords = owner->coords;
    // Spawn IDs 19 and 21 are the native ordinary and fire orc archers.
    Actor *foe = actor_enemySpawner_spawnChild(spot & 1 ? 21 : 19, &spawner);
    if (foe == NULL)
    {
        return;
    }
    foe->parentActor = NULL;
    foe->recordGroup = foe->recordIndex = 0xff;
    if (!hackLib_findLanding(foe, &preferred, &landing))
    {
        foe->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
        return;
    }
    foe->renderCoords.x += landing.x - foe->coords.x;
    foe->renderCoords.y += landing.y - foe->coords.y;
    foe->coords = landing;
}

static char *appendHex(char *output, u32 value)
{
    *output++ = ' ';
    for (s32 shift = 28; shift >= 0; shift -= 4)
    {
        u32 digit = (value >> shift) & 0xf;
        *output++ = digit < 10 ? '0' + digit : 'a' + digit - 10;
    }
    *output = 0;
    return output;
}

static void samplerPrepare(void)
{
    testArena_samples = memory_zalloc(sizeof(*testArena_samples));
    if (testArena_samples == NULL)
    {
        debugPrint("ARENA_PC_UNAVAILABLE 1");
    }
}

static void samplerStart(void)
{
    if (testArena_samples == NULL)
    {
        return;
    }
    debugPrint("ARENA_PC_START");
    u16 ime = REG_IME;
    REG_IME = 0;
    testArena_samples->chain = BIOS_ISR_POINTER;
    BIOS_ISR_POINTER = sampleIsr;
    REG_TM2CNT_H = 0;
    REG_TM2CNT_L = SAMPLE_TIMER_RELOAD;
    REG_TM2CNT_H = SAMPLE_TIMER_START;
    REG_IE |= SAMPLE_IRQ_TIMER2;
    REG_IME = ime;
}

/** Print buffered samples as ARENA_PC lines of up to 16 program counters. */
static void samplerFlush(void)
{
    SampleBuffer *buffer = testArena_samples;
    u32 pcs[SAMPLE_CAPACITY];
    char line[DEBUG_STRING_SIZE];

    if (buffer == NULL || buffer->chain == NULL)
    {
        return;
    }
    REG_IE |= SAMPLE_IRQ_TIMER2;
    u16 ime = REG_IME;
    REG_IME = 0;
    u32 count = buffer->count;
    memory_memcpy(pcs, buffer->pcs, count * sizeof(pcs[0]));
    buffer->count = 0;
    REG_IME = ime;
    for (u32 i = 0; i < count; i += 16)
    {
        char *output = appendText(line, "ARENA_PC");
        for (u32 j = i; j < count && j < i + 16; j++)
        {
            output = appendHex(output, pcs[j]);
        }
        debugPrint(line);
    }
}

static void samplerStop(void)
{
    if (testArena_samples == NULL)
    {
        return;
    }
    if (testArena_samples->chain != NULL)
    {
        samplerFlush();
        u16 ime = REG_IME;
        REG_IME = 0;
        REG_TM2CNT_H = 0;
        REG_IE &= ~SAMPLE_IRQ_TIMER2;
        REG_IF = SAMPLE_IRQ_TIMER2;
        BIOS_ISR_POINTER = testArena_samples->chain;
        REG_IME = ime;
        char line[DEBUG_STRING_SIZE];
        appendNumber(appendText(line, "ARENA_PC_DROPPED"), testArena_samples->dropped);
        debugPrint(line);
    }
    memory_free(testArena_samples);
    testArena_samples = NULL;
}

static void benchPrintResults(void)
{
    volatile TestArenaControl *arena = g_TestArena;
    char line[DEBUG_STRING_SIZE];
    char *output = appendText(line, "ARENA_BENCH");

    output = appendNumber(output, arena->ticks - arena->benchPhaseTick);
    output = appendNumber(output, g_EngineState->vblankCounter - arena->benchStartVblank);
    output = appendNumber(output, arena->lagTicks - arena->benchStartLag);
    output = appendNumber(output, arena->worstTickVblanks);
    output = appendNumber(output, arena->benchFoeTicks);
    appendNumber(output, arena->benchCasts);
    debugPrint(line);
    for (u32 section = 0; section < HACKLIB_PROFILE_SECTION_COUNT; section++)
    {
        HackLibProfileRecord record;
        if (!hackLib_getProfile(section, &record))
        {
            break;
        }
        output = appendText(line, "ARENA_PROFILE");
        output = appendNumber(output, section);
        output = appendNumber(output, record.calls);
        output = appendNumber(output, record.totalLines);
        output = appendNumber(output, record.peakLines);
        appendNumber(output, record.work);
        debugPrint(line);
    }
}

/**
 * Busy fight benchmark: keep a crowd of foes alive around an unkillable player who casts
 * whatever is castable, then report lag over benchTicks.
 */
static void benchTick(void)
{
    volatile TestArenaControl *arena = g_TestArena;
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[0];

    if (arena->benchPhase == 3)
    {
        return;
    }
    if (arena->benchPhase == 0)
    {
        debugLab_clearTargets();
        samplerPrepare();
        hackLib_enableProfiler(TRUE);
        arena->benchPhase = 1;
        arena->benchPhaseTick = arena->ticks;
    }
    refill();
    memory_memset(runtime->cooldowns, 0, sizeof(runtime->cooldowns));
    u32 foes = countFoes();
    for (u32 i = 0; i < BENCH_SPAWNS_PER_TICK && foes + i < testArena_config.benchCrowd; i++)
    {
        benchSpawnFoe();
    }
    bool cast = FALSE;
    if (!runtime->pending)
    {
        for (u32 i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT && !cast; i++)
        {
            u32 row = (arena->benchRow + i) % SKILL_EXPANSION_ACTIVE_ROW_COUNT;
            if (skillExpansion_activeCanCast(0, row))
            {
                skillExpansion_cast(0, row);
                arena->benchRow = row + 1;
                cast = TRUE;
            }
        }
    }
    if (arena->benchPhase == 1 && arena->ticks - arena->benchPhaseTick >= BENCH_WARMUP_TICKS)
    {
        arena->benchPhase = 2;
        arena->benchPhaseTick = arena->ticks;
        arena->benchStartVblank = g_EngineState->vblankCounter;
        arena->benchStartLag = arena->lagTicks;
        arena->worstTickVblanks = 0;
        hackLib_resetProfiler();
        debugPrint("ARENA_BENCH_START");
        if (testArena_config.benchSample)
        {
            samplerStart();
        }
    }
    else if (arena->benchPhase == 2)
    {
        arena->benchFoeTicks += foes;
        arena->benchCasts += cast;
        samplerFlush();
        if (arena->ticks - arena->benchPhaseTick >= testArena_config.benchTicks)
        {
            samplerStop();
            benchPrintResults();
            arena->benchPhase = 3;
        }
    }
}

__attribute__((noinline)) void testArena_checkpoint(void)
{
    asm volatile("nop");
}

__attribute__((noinline)) void testArena_acknowledgedPoint(void)
{
    asm volatile("nop");
}

void testArena_tick(u8 p)
{
    if (p != 0 || g_SceneCurrent.id != SCENE_ID_GAMEPLAY || g_GameFlags.d.overlay ||
        (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) || PLAYER(0).ownerActor == NULL)
    {
        return;
    }
    if (g_TestArena->ready == 0)
    {
        g_TestArena->ready = findAnchor(PLAYER(0).ownerActor) && reset(0) ? 1 : 2;
        if (g_TestArena->ready == 2 && testArena_config.sweepTicks != 0)
        {
            debugPrint("ARENA_FAIL 2");
        }
    }
    g_TestArena->ticks++;
    u32 vblank = g_EngineState->vblankCounter;
    if (g_TestArena->lastTickVblank != 0)
    {
        u32 elapsed = vblank - g_TestArena->lastTickVblank;
        if (elapsed > 2)
        {
            g_TestArena->lagTicks++;
            if (testArena_config.sweepTicks != 0)
            {
                sweepPrintLag(elapsed);
            }
        }
        if (elapsed > g_TestArena->worstTickVblanks)
        {
            g_TestArena->worstTickVblanks = elapsed;
        }
    }
    g_TestArena->lastTickVblank = vblank;
    bool acknowledged = FALSE;
    if (g_TestArena->ready == 1 && testArena_config.benchTicks != 0)
    {
        benchTick();
    }
    else if (g_TestArena->ready == 1)
    {
        sweepAdvance();
    }
    if (g_TestArena->ready == 1 && g_TestArena->request != g_TestArena->acknowledged)
    {
        u32 request = g_TestArena->request;
        u32 command = g_TestArena->command, argument = g_TestArena->argument;
        u32 level = g_TestArena->level;
        bool result = FALSE;
        bool done = TRUE;
        switch (command)
        {
        case TEST_ARENA_RESET:
            result = reset(argument);
            g_TestArena->resetTick = g_TestArena->ticks;
#if PC_EDITION
            if (testArena_config.legacyLevel != 0)
            {
                legacySkills_setTestLevels(testArena_config.legacyLevel);
            }
#endif
            break;
        case TEST_ARENA_DELIVERY:
            if (argument < SKILL_DELIVERY_COUNT && level >= 1 && level <= 5)
            {
                Actor *owner = PLAYER(0).ownerActor;
                result = owner->facing < 8 &&
                         skillDelivery_launch(0, &SkillDeliveryDefinitions[argument],
                                              &owner->coords, owner->facing * 4, level,
                                              HACKLIB_ABILITY_NONE, g_TestArena->cosmetic != 0);
            }
            break;
        case TEST_ARENA_CAST:
            if (argument < castRowEnd() && skillExpansion_activeCanCast(0, argument))
            {
                skillExpansion_cast(0, argument);
                result = TRUE;
            }
            break;
        case TEST_ARENA_REFILL:
            refill();
            result = TRUE;
            break;
        case TEST_ARENA_CLEAR:
            skillDelivery_clear(0);
            hackLib_clearOwnerEffects(PLAYER(0).ownerActor);
            hackLib_clearOwnerSummons(PLAYER(0).ownerActor);
            debugLab_clearTargets();
            result = TRUE;
            break;
        case TEST_ARENA_WAIT:
            if (g_TestArena->waitUntil == 0)
            {
                g_TestArena->waitUntil = g_TestArena->ticks + argument;
            }
            done = g_TestArena->ticks >= g_TestArena->waitUntil;
            if (done)
            {
                g_TestArena->waitUntil = 0;
                result = TRUE;
            }
            break;
        }
        if (done)
        {
            g_TestArena->result = result;
            g_TestArena->acknowledged = request;
            acknowledged = TRUE;
        }
    }
    const DebugLabCombatStats *stats = debugLab_getCombatStats(0);
    g_TestArena->hits = stats->hits;
    g_TestArena->kills = stats->kills;
    g_TestArena->damage = stats->damage;
    g_TestArena->playerHp = PLAYER(0).currentHp;
    g_TestArena->playerSpirit = PLAYER(0).currentSpirit;
    for (u32 i = 0; i < 3; i++)
    {
        Actor *target = debugLab_getTarget(i);
        g_TestArena->targets[i] = target;
        g_TestArena->targetHp[i] = target != NULL ? target->currentHp : 0;
    }
    ModRamMemoryStats memory;
    modRam_getMemoryStats(&memory);
    g_TestArena->heapRoom = memory.heapEnd - memory.peakTop;
    g_TestArena->allocFailures = memory.failures;
    g_TestArena->invalidFrees = memory.invalidFrees;
    g_TestArena->bssGuardIntact = modRam_isBssGuardIntact();
    g_TestArena->heroStateMisuse = skillExpansion_heroStateMisuse;
    if (PLAYER(0).currentHp <= 0)
    {
        g_TestArena->sweepValues[TEST_ARENA_SWEEP_PLAYER_DOWNED] = 1;
    }
    testArena_checkpoint();
    if (acknowledged)
    {
        testArena_acknowledgedPoint();
    }
}
