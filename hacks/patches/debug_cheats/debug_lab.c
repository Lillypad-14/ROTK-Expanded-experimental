#include "patches/debug_cheats/debug_lab.h"
#include "actor/enemySpawner.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "lib/movement.h"
#include "lib/profile.h"
#include "lib/summon_registry.h"
#include "lib/timed_effect.h"
#include "memory.h"
#include "patches/debug_cheats/ability_bench.h"
#include "patches/debug_cheats/delivery_preview.h"
#include "patches/debug_cheats/effect_preview.h"
#include "patches/mod_ram/graphics_scratch.h"
#include "patches/mod_ram/memory_diagnostics.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "scene.h"
#include "stats.h"
#include "text.h"
#if HACK_test_arena
#include "patches/test_arena/test_arena.h"
#endif

#define LAB_TARGET_LIMIT 3
#define LAB_DUMMY_HP 30000

typedef struct LabTarget
{
    Actor *actor;
    u32 generation;
    ActorCallback originalTick;
    bool dummy;
} LabTarget;

static LabTarget targets[LAB_TARGET_LIMIT];
static u8 displayMode;
static u8 spawnRequest;
static u8 refillRequest;
static u8 clearRequest;
static u8 encounter;

extern u8 __modcode_bss_start[];
extern u8 __modcode_bss_end[];

static void tickTarget(Actor *actor)
{
    u32 i;

    for (i = 0; i < LAB_TARGET_LIMIT; i++)
    {
        if (targets[i].actor == actor)
        {
            if (targets[i].dummy && actor->currentHp > 0)
            {
                actor->velocity.x = actor->velocity.y = 0;
                actor->triggerBoxes[COMBAT_TRIGGER_HITBOX].enable = FALSE;
                return;
            }
            if (targets[i].originalTick != NULL)
            {
                targets[i].originalTick(actor);
            }
            return;
        }
    }
}

static bool isLiveTarget(Actor *actor)
{
    Actor *cursor;
    bool valid = FALSE;
    for (u32 i = 0; i < LAB_TARGET_LIMIT; i++)
    {
        if (targets[i].actor == actor &&
            hackLib_isActorRefValid((HackLibActorRef){actor, targets[i].generation}))
        {
            valid = TRUE;
        }
    }
    if (!valid)
    {
        return FALSE;
    }

    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        if (cursor == actor)
        {
            return hackLib_actorHasTick(cursor, tickTarget) &&
                   !(cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
        }
    }
    return FALSE;
}

void debugLab_clearTargets(void)
{
    u32 i;

    for (i = 0; i < LAB_TARGET_LIMIT; i++)
    {
        if (isLiveTarget(targets[i].actor))
        {
            targets[i].actor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
        }
    }
    memory_memset(targets, 0, sizeof(targets));
}

u32 debugLab_prepareEncounter(u8 p, u8 kind)
{
    if (p >= 2 || kind > 2 || PLAYER(p).ownerActor == NULL ||
        (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE))
    {
        return 0;
    }
    Actor *owner = PLAYER(p).ownerActor;
    Actor spawner = {0};
    Actor *foe = NULL;
    u32 count = kind == 1 ? LAB_TARGET_LIMIT : 1;
    u32 spawned = 0;
    u32 i;

    if (!(g_GameFlags.p & GAME_FLAG_BONUS_MAP_ACTIVE))
    {
        skillExpansion_feedback(p, "Combat Lab: enter a bonus map first");
        return 0;
    }
    while ((foe = findNextEnemy(owner, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && !(foe->flags.p & ACTOR_FLAG_PENDING_REMOVE) &&
            !isLiveTarget(foe))
        {
            skillExpansion_feedback(p, "Combat Lab: clear nearby foes first");
            return 0;
        }
    }
    debugLab_clearTargets();
    spawner.coords = owner->coords;
    for (i = 0; i < count; i++)
    {
        Actor *target;
        Vector2Fp16 preferred = {owner->coords.x + (32 + (s32)i * 24) * FP16_16_ONE,
                                 owner->coords.y};
        Vector2Fp16 landing;

        // Spawn IDs 19 and 21 are the native ordinary and fire orc archers.
        target = actor_enemySpawner_spawnChild(kind == 2 ? 21 : 19, &spawner);
        if (target == NULL)
        {
            break;
        }
        target->parentActor = NULL;
        target->recordGroup = target->recordIndex = 0xff;
        if (!hackLib_findLanding(target, &preferred, &landing))
        {
            target->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
            continue;
        }
        target->renderCoords.x += landing.x - target->coords.x;
        target->renderCoords.y += landing.y - target->coords.y;
        target->coords = landing;
        targets[i].actor = target;
        targets[i].generation = hackLib_actorGeneration(target);
        targets[i].originalTick = target->tickCallback;
        targets[i].dummy = kind == 0;
        spawned++;
        target->tickCallback = tickTarget;
        if (targets[i].dummy)
        {
            target->currentHp = LAB_DUMMY_HP;
            target->triggerBoxes[COMBAT_TRIGGER_HITBOX].enable = FALSE;
        }
    }
    skillExpansion_feedback(p, "Combat Lab ready");
    return spawned;
}

Actor *debugLab_getTarget(u8 index)
{
    return index < LAB_TARGET_LIMIT && isLiveTarget(targets[index].actor) ? targets[index].actor
                                                                          : NULL;
}

static void showTelemetry(u8 p)
{
    char *text = g_PlayerHuds[p].textBuffer;
    Actor *ghost = NULL;
    u32 count = 0;
    u32 minimum = 0xffff;
    const char *prefix = "Hit ";

    while ((ghost = hackLib_nextOwnedGhost(PLAYER(p).ownerActor, ghost)) != NULL)
    {
        count++;
        if (ghostState(ghost)->lifetime < minimum)
        {
            minimum = ghostState(ghost)->lifetime;
        }
    }
    while (*prefix)
    {
        *text++ = *prefix++;
    }
    text = text_formatInt(debugLab_getCombatStats(p)->lastDamage, text);
    while (*text)
    {
        text++;
    }
    *text++ = '/';
    *text++ = '0' + debugLab_getCombatStats(p)->lastKind;
    *text++ = ' ';
    *text++ = 'S';
    *text++ = ':';
    text = text_formatInt(count, text);
    while (*text)
    {
        text++;
    }
    *text++ = '/';
    text = text_formatInt(count != 0 ? minimum / 60 : 0, text);
    while (*text)
    {
        text++;
    }
    *text++ = 's';
    *text = 0;
    g_PlayerHuds[p].text = g_PlayerHuds[p].textBuffer;
    g_PlayerHuds[p].textColor = HUD_TEXT_COLOR_GAMEPLAY_WHITE;
    g_PlayerHuds[p].flags.p |= PLAYERHUD_FLAG_0 | PLAYERHUD_FLAG_1;
}

static char *appendText(char *output, const char *value)
{
    while (*value)
    {
        *output++ = *value++;
    }
    *output = 0;
    return output;
}

static char *appendNumber(char *output, s32 value)
{
    text_formatInt(value, output);
    while (*output)
    {
        output++;
    }
    return output;
}

static void showEngineTelemetry(u8 p)
{
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[p];
    char *output = g_PlayerHuds[p].textBuffer;
    u32 row = PLAYER(p).selectedActiveSkill.cycleSkillIndex;

    if (displayMode == 14)
    {
        const DebugLabCombatStats *stats = debugLab_getCombatStats(p);
        output = appendText(output, "A:");
        output = appendNumber(output, stats->attempts);
        output = appendText(output, " H:");
        output = appendNumber(output, stats->hits);
        output = appendText(output, " K:");
        output = appendNumber(output, stats->kills);
        output = appendText(output, " D:");
        appendNumber(output, stats->damage);
    }
    else if (displayMode == 2)
    {
        output = appendText(output, "Cast:");
        output = appendNumber(output, runtime->pending ? runtime->pendingRow : -1);
        output = appendText(output, " Paid:");
        output = appendNumber(output, runtime->paidSpirit);
        output = appendText(output, " Result:");
        output = appendNumber(output, runtime->lastResult);
        output = appendText(output, " CD:");
        appendNumber(output, row < SKILL_EXPANSION_ACTIVE_ROW_COUNT ? runtime->cooldowns[row] : 0);
    }
    else if (displayMode == 3)
    {
        const HackLibTimedEffect *first = NULL;
        output = appendText(output, "Effects:");
        output = appendNumber(output, hackLib_countEffects(PLAYER(p).ownerActor));
        for (u32 i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
        {
            const HackLibTimedEffect *effect = hackLib_effectAt(i);
            if (effect != NULL && effect->owner == PLAYER(p).ownerActor)
            {
                first = effect;
                break;
            }
        }
        if (first != NULL)
        {
            output = appendText(output, " ID:");
            output = appendNumber(output, first->definition->id);
            output = appendText(output, " Frames:");
            output = appendNumber(output, first->remaining);
            output = appendText(output, " Stacks:");
            appendNumber(output, first->stacks);
        }
    }
    else if (displayMode == 4)
    {
        output = appendText(output, "Pools E:");
        output = appendNumber(output, hackLib_effectPoolUsed());
        output = appendText(output, "/12 S:");
        output = appendNumber(output, hackLib_summonPoolUsed());
        output = appendText(output, "/8 P:");
        output = appendNumber(output, skillExpansion_projectilePoolUsed());
        appendText(output, "/8");
    }
    else if (displayMode >= 7)
    {
        HackLibProfileRecord record;
        output = appendText(output, "CPU lines:");
        if (hackLib_getProfile(displayMode - 7, &record))
        {
            output = appendNumber(output, record.lastLines);
            output = appendText(output, " Peak:");
            output = appendNumber(output, record.peakLines);
            output = appendText(output, " Work:");
            appendNumber(output, record.work);
        }
    }
    else if (displayMode == 6)
    {
        ModRamMemoryStats stats;
        modRam_getMemoryStats(&stats);
        output = appendText(output, "Fail:");
        output = appendNumber(output, stats.failures);
        output = appendText(output, " Size:");
        output = appendNumber(output, stats.lastFailureSize);
        output = appendText(output, " Reuse:");
        appendNumber(output, modRam_sharedTextUses());
    }
    else
    {
        ModRamMemoryStats stats;
        modRam_getMemoryStats(&stats);
        output = appendText(output, "Heap:");
        output = appendNumber(output, stats.freeBytes);
        output = appendText(output, " Max:");
        output = appendNumber(output, stats.largestBlock);
        output = appendText(output, " Peak:");
        appendNumber(output, stats.peakUsed);
    }
    g_PlayerHuds[p].text = g_PlayerHuds[p].textBuffer;
    g_PlayerHuds[p].textColor = HUD_TEXT_COLOR_GAMEPLAY_WHITE;
    g_PlayerHuds[p].flags.p |= PLAYERHUD_FLAG_0 | PLAYERHUD_FLAG_1;
}

void debugLab_playerTick(u8 playerIndex)
{
    abilityBench_playerTick(playerIndex);
    deliveryPreview_playerTick(playerIndex);
    effectPreview_playerTick(playerIndex);
    if (g_SceneCurrent.id != SCENE_ID_GAMEPLAY || playerIndex != ACTIVE_PLAYER_INDEX ||
        g_GameFlags.d.overlay)
    {
        return;
    }
    if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE)
    {
        spawnRequest = refillRequest = clearRequest = 0;
        return;
    }
    if (clearRequest)
    {
        clearRequest = 0;
        debugLab_clearTargets();
    }
    if (spawnRequest)
    {
        spawnRequest = 0;
        debugLab_prepareEncounter(playerIndex, encounter);
    }
    if (refillRequest)
    {
        refillRequest = 0;
        PLAYER(playerIndex).currentHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
        if (skillExpansion_runtime[playerIndex].kit != NULL &&
            skillExpansion_runtime[playerIndex].kit->refundSpirit != NULL)
        {
            skillExpansion_runtime[playerIndex].kit->refundSpirit(
                playerIndex,
                PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) - PLAYER(playerIndex).currentSpirit);
        }
        else
        {
            PLAYER(playerIndex).currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
        }
    }
#if HACK_test_arena
    testArena_tick(playerIndex);
#endif
}

void debugLab_drawTelemetry(u8 playerIndex)
{
    if (displayMode && !(g_GameFlags.p & GAME_FLAG_COOP_ACTIVE))
    {
        if (displayMode == 1)
        {
            showTelemetry(playerIndex);
        }
        else
        {
            showEngineTelemetry(playerIndex);
        }
    }
}

#if HACK_mod_options
static const char *const Displays[] = {"Off",          "Combat",   "Casts",       "Effects",
                                       "Pools",        "Memory",   "Allocations", "CPU Player",
                                       "CPU Passives", "CPU Hero", "CPU Targets", "CPU Effects",
                                       "CPU Summons",  "CPU FX",   "Events"};
static const char *const Actions[] = {"Idle", "Run"};
static const char *const Encounters[] = {"Dummy", "Group", "Fire"};

#define LAB_OPTION(name, field)                                                                    \
    static u8 get##name(void)                                                                      \
    {                                                                                              \
        return field;                                                                              \
    }                                                                                              \
    static void set##name(u8 value)                                                                \
    {                                                                                              \
        field = value;                                                                             \
    }

static u8 getDisplay(void)
{
    return displayMode;
}
static void setDisplay(u8 value)
{
    if (hackLib_enableProfiler(value >= 7 && value <= 13))
    {
        displayMode = value;
    }
}
LAB_OPTION(Encounter, encounter)
LAB_OPTION(Spawn, spawnRequest)
LAB_OPTION(Refill, refillRequest)
LAB_OPTION(Clear, clearRequest)
static u8 getResetEvents(void)
{
    return 0;
}
static void setResetEvents(u8 value)
{
    if (value)
    {
        debugLab_resetCombatStats();
    }
}

const struct ModOptionEntry DebugLabOptions[DEBUG_LAB_OPTION_COUNT] = {
    {"Combat display", Displays, ARRAY_COUNT(Displays), 0, getDisplay, setDisplay},
    {"Encounter", Encounters, 3, 0, getEncounter, setEncounter},
    {"Spawn encounter", Actions, 2, 0, getSpawn, setSpawn},
    {"Refill HP/Spirit", Actions, 2, 0, getRefill, setRefill},
    {"Clear targets", Actions, 2, 0, getClear, setClear},
    {"Reset events", Actions, 2, 0, getResetEvents, setResetEvents},
};
#endif
