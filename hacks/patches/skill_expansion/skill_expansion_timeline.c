#include "patches/skill_expansion/skill_expansion_timeline.h"
#include "game.h"
#include "lib/skill_fx.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "scene.h"
#if HACK_debug_cheats
#include "patches/debug_cheats/ability_bench.h"
#endif

typedef struct CastTimelineState
{
    HackLibTimelineCursor cursor;
    HackLibActorRef owner;
    HackLibActorRef target;
    HackLibEffectHandle visual;
    Vector2Fp16 position;
    u8 player;
    u8 hero;
    bool stepping;
    bool cancelled;
} CastTimelineState;

static CastTimelineState *states[2];

const HackLibCastTimeline *skillExpansion_getTimeline(u8 playerIndex, u8 row)
{
#if HACK_debug_cheats
    const HackLibCastTimeline *override = abilityBench_getTimeline(playerIndex, row);
    if (override != NULL)
    {
        return override;
    }
#endif
    const SkillExpansionActiveDef *definition = skillExpansion_activeDef(playerIndex, row);
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(PLAYER(playerIndex).heroId);
    return definition != NULL && definition->timeline != NULL ? definition->timeline
           : kit != NULL && kit->timelines != NULL            ? kit->timelines[row]
                                                              : NULL;
}

bool skillExpansion_isTimelineBusy(u8 playerIndex)
{
    return playerIndex < 2 && states[playerIndex] != NULL;
}

void skillExpansion_clearTimeline(u8 playerIndex)
{
    CastTimelineState *state = states[playerIndex];
    if (state == NULL)
    {
        return;
    }
    states[playerIndex] = NULL;
    hackLib_stopEffect(&state->visual);
    if (state->stepping)
    {
        state->cancelled = TRUE;
    }
    else
    {
        memory_free(state);
    }
}

bool skillExpansion_prepareTimeline(u8 playerIndex, Actor *actor,
                                    const HackLibCastTimeline *definition)
{
    CastTimelineState *state = memory_zalloc(sizeof(*state));
    if (state == NULL)
    {
        return FALSE;
    }
    if (!hackLib_startTimeline(&state->cursor, definition))
    {
        memory_free(state);
        return FALSE;
    }
    skillExpansion_clearTimeline(playerIndex);
    state->owner = hackLib_actorRef(actor);
    state->target = hackLib_actorRef(skillExpansion_findTarget(actor, 160, NULL, 0));
    state->position = actor->coords;
    state->player = playerIndex;
    state->hero = PLAYER(playerIndex).heroId;
    states[playerIndex] = state;
    return TRUE;
}

static bool emit(u8 kind, const HackLibTimelineEvent *event, void *context)
{
    CastTimelineState *state = context;
    if (state->cancelled)
    {
        return FALSE;
    }
    if (kind == HACKLIB_TIMELINE_RELEASE)
    {
        skillExpansion_resolveTimelineCast(state->owner.actor);
        return !state->cancelled &&
               skillExpansion_runtime[state->player].lastResult == SKILL_CAST_SUCCESS;
    }
    if (kind == HACKLIB_TIMELINE_VISUAL)
    {
        Actor *on = state->owner.actor;
        hackLib_stopEffect(&state->visual);
        if (event->anchor == HACKLIB_TIMELINE_TARGET)
        {
            if (!hackLib_isActorRefValid(state->target))
            {
                return TRUE;
            }
            on = state->target.actor;
        }
        hackLib_playEffect(event->effect, on,
                           event->anchor == HACKLIB_TIMELINE_POSITION ? &state->position : NULL,
                           &state->visual);
    }
    return TRUE;
}

void skillExpansion_tickTimeline(u8 playerIndex, Actor *actor)
{
    CastTimelineState *state = states[playerIndex];
    if (state == NULL || g_GameFlags.d.overlay)
    {
        return;
    }
    if (state->owner.actor != actor || !hackLib_isActorRefValid(state->owner) ||
        state->hero != PLAYER(playerIndex).heroId || g_SceneCurrent.id != SCENE_ID_GAMEPLAY ||
        hackLib_actorCurrentHp(actor) <= 0 || actor->actionState == ACTOR_STATE_STUNNED ||
        actor->actionState == ACTOR_STATE_HURT || actor->actionState == ACTOR_STATE_KNOCKBACK ||
        actor->actionState == ACTOR_STATE_KNOCKED_DOWN)
    {
        if (skillExpansion_runtime[playerIndex].pending)
        {
            skillExpansion_finishCast(playerIndex, SKILL_CAST_INTERRUPTED);
        }
        skillExpansion_clearTimeline(playerIndex);
        return;
    }
    state->stepping = TRUE;
    bool running = hackLib_stepTimeline(&state->cursor, emit, state);
    state->stepping = FALSE;
    if (!running || state->cancelled)
    {
        if (states[playerIndex] == state)
        {
            states[playerIndex] = NULL;
        }
        hackLib_stopEffect(&state->visual);
        memory_free(state);
    }
}
