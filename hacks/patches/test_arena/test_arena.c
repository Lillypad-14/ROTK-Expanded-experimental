#include "patches/test_arena/test_arena.h"
#include "game.h"
#include "lib/movement.h"
#include "lib/summon_registry.h"
#include "lib/timed_effect.h"
#include "memory.h"
#include "mission.h"
#include "patches/debug_cheats/debug_cheats.h"
#include "patches/debug_cheats/debug_lab.h"
#include "patches/skill_expansion/skill_delivery.h"
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

#if !HACK_debug_cheats || !HACK_skill_expansion
#error "test_arena requires debug_cheats and skill_expansion"
#endif
#ifndef TEST_ARENA_HERO
#define TEST_ARENA_HERO HERO_ID_ARAGORN
#endif
_Static_assert(TEST_ARENA_HERO < SKILL_EXPANSION_HERO_COUNT, "invalid arena hero");

volatile TestArenaControl *g_TestArena;
static Vector2Fp16 anchor;

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
    g_TestArena = memory_zalloc(sizeof(*g_TestArena));
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
    PLAYER(0).heroId = TEST_ARENA_HERO;
    for (u32 i = 0; i < ARRAY_COUNT(PLAYER(0).inventory.array); i++)
    {
        item_clearSlot(&PLAYER(0).inventory.array[i].word);
    }
    player_initHeroStats(TEST_ARENA_HERO, 0);
    player_initHeroLoadout(TEST_ARENA_HERO, 0);
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
    g_MissionVariant = 0;
    g_MissionSpawnPoint[0] = g_MissionSpawnPoint[1] = 0;
    *g_TestArena = (TestArenaControl){.magic = 0x41524e41, .version = 1, .level = 1};
#if HACK_fast_waitstates
    fastWaitstates_apply();
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

__attribute__((noinline)) void testArena_checkpoint(void)
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
    }
    g_TestArena->ticks++;
    if (g_TestArena->ready == 1 && g_TestArena->request != g_TestArena->acknowledged)
    {
        u32 request = g_TestArena->request;
        u32 command = g_TestArena->command, argument = g_TestArena->argument;
        u32 level = g_TestArena->level;
        bool result = FALSE;
        switch (command)
        {
        case TEST_ARENA_RESET:
            result = reset(argument);
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
            if (argument < SKILL_EXPANSION_ACTIVE_ROW_COUNT &&
                skillExpansion_activeCanCast(0, argument))
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
        }
        g_TestArena->result = result;
        g_TestArena->acknowledged = request;
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
    testArena_checkpoint();
}
