#include "patches/debug_cheats/ability_bench.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "patches/skill_expansion/skill_expansion_timeline.h"
#include "scene.h"
#include "stats.h"

typedef struct BenchSession
{
    HackLibActorRef owner;
    u8 hero;
    u8 player;
    u8 row;
    u8 savedLevel;
} BenchSession;

static BenchSession *session;
static u8 selected;
static u8 level;
static u8 timing;
static u8 practice;
static u8 castRequest;
static u8 cancelRequest;
static u8 resetRequest;

static u8 selectedRow(void)
{
    return selected < ACTIVE_SKILL_NONE ? selected : selected + 1;
}

static bool isSameOwner(u8 playerIndex)
{
    return session != NULL && session->player == playerIndex &&
           session->hero == PLAYER(playerIndex).heroId &&
           session->owner.actor == PLAYER(playerIndex).ownerActor &&
           hackLib_isActorRefValid(session->owner);
}

void abilityBench_endSession(void)
{
    if (session != NULL)
    {
        if (isSameOwner(session->player))
        {
            *skillExpansion_activeLevel(session->player, session->row) = session->savedLevel;
        }
        memory_free(session);
        session = NULL;
    }
}

const HackLibCastTimeline *abilityBench_getTimeline(u8 playerIndex, u8 row)
{
    return isSameOwner(playerIndex) && session->row == row && timing > 0 &&
                   timing <= HACKLIB_CAST_TIMELINE_COUNT
               ? &HackLibCastTimelines[timing - 1]
               : NULL;
}

void abilityBench_playerTick(u8 playerIndex)
{
    if (playerIndex != ACTIVE_PLAYER_INDEX)
    {
        return;
    }
    if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE || !(g_GameFlags.p & GAME_FLAG_BONUS_MAP_ACTIVE))
    {
        if (practice || castRequest || cancelRequest || resetRequest)
        {
            skillExpansion_feedback(playerIndex, "Ability Bench: single-player bonus maps only");
        }
        practice = castRequest = cancelRequest = resetRequest = 0;
        abilityBench_endSession();
        return;
    }
    if (g_SceneCurrent.id != SCENE_ID_GAMEPLAY || g_GameFlags.d.overlay)
    {
        return;
    }
    Actor *actor = PLAYER(playerIndex).ownerActor;
    if (session != NULL && !isSameOwner(playerIndex))
    {
        abilityBench_endSession();
    }
    if (actor == NULL || hackLib_actorCurrentHp(actor) <= 0)
    {
        practice = castRequest = cancelRequest = resetRequest = 0;
        abilityBench_endSession();
        return;
    }
    SkillExpansionRuntime *runtime = &skillExpansion_runtime[playerIndex];
    if (cancelRequest)
    {
        cancelRequest = 0;
        if (runtime->pending)
        {
            runtime->timelineCast = TRUE;
            skillExpansion_finishCast(playerIndex, SKILL_CAST_INTERRUPTED);
        }
        skillExpansion_clearTimeline(playerIndex);
    }
    if (practice || resetRequest)
    {
        memory_memset(runtime->cooldowns, 0, sizeof(runtime->cooldowns));
        resetRequest = 0;
    }
    if (practice)
    {
        PLAYER(playerIndex).currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    }
    bool casting = runtime->pending || skillExpansion_isTimelineBusy(playerIndex) ||
                   (PLAYER(playerIndex).activeSkillStateFlags & ACTIVE_SKILL_STATE_FLAG_CASTING);
    if (session != NULL && !casting && (!practice || session->row != selectedRow()))
    {
        abilityBench_endSession();
    }
    if (casting)
    {
        if (castRequest)
        {
            castRequest = 0;
            skillExpansion_feedback(playerIndex, "Ability Bench: cast still active");
        }
        return;
    }
    u8 row = selectedRow();
    if ((practice || castRequest) && row < SKILL_EXPANSION_ACTIVE_ROW_COUNT &&
        PLAYER_ACTIVE_SKILL(playerIndex, row).callback != NULL)
    {
        if (session == NULL)
        {
            session = memory_zalloc(sizeof(*session));
            if (session == NULL)
            {
                practice = castRequest = 0;
                skillExpansion_feedback(playerIndex, "Ability Bench: no memory");
                return;
            }
            *session =
                (BenchSession){hackLib_actorRef(actor), PLAYER(playerIndex).heroId, playerIndex,
                               row, *skillExpansion_activeLevel(playerIndex, row)};
        }
        *skillExpansion_activeLevel(playerIndex, row) = level + 1;
    }
    if (castRequest)
    {
        castRequest = 0;
        skillExpansion_cast(playerIndex, row);
    }
}

#if HACK_mod_options
static const char *const Skills[] = {"Page 1: 1", "Page 1: 2", "Page 1: 3", "Page 1: 4",
                                     "Page 1: 5", "Healing",   "Page 2: 1", "Page 2: 2",
                                     "Page 2: 3", "Page 2: 4", "Page 2: 5", "Page 2: 6"};
_Static_assert(ARRAY_COUNT(Skills) == SKILL_EXPANSION_ACTIVE_ROW_COUNT - 1,
               "update bench skill labels");
static const char *const Levels[] = {"1", "2", "3", "4", "5"};
static const char *const Toggles[] = {"Off", "On"};
static const char *const Actions[] = {"Idle", "Run"};
#define OPTION(name, field)                                                                        \
    static u8 get##name(void)                                                                      \
    {                                                                                              \
        return field;                                                                              \
    }                                                                                              \
    static void set##name(u8 value)                                                                \
    {                                                                                              \
        field = value;                                                                             \
    }
OPTION(Skill, selected)
OPTION(Level, level)
OPTION(Timing, timing)
OPTION(Practice, practice)
OPTION(Cast, castRequest)
OPTION(Cancel, cancelRequest)
OPTION(Reset, resetRequest)
const struct ModOptionEntry AbilityBenchOptions[ABILITY_BENCH_OPTION_COUNT] = {
    {"Skill", Skills, ARRAY_COUNT(Skills), 0, getSkill, setSkill, .desc = NULL},
    {"Test level", Levels, ARRAY_COUNT(Levels), 0, getLevel, setLevel, .desc = NULL},
    {"Cast timing", HackLibCastTimelineNames, HACKLIB_CAST_TIMELINE_COUNT + 1, 0, getTiming,
     setTiming, .desc = NULL},
    {"Practice", Toggles, 2, 0, getPractice, setPractice, .desc = NULL},
    {"Cast on resume", Actions, 2, 0, getCast, setCast, .desc = NULL},
    {"Cancel cast", Actions, 2, 0, getCancel, setCancel, .desc = NULL},
    {"Reset cooldowns", Actions, 2, 0, getReset, setReset, .desc = NULL},
};
#endif
