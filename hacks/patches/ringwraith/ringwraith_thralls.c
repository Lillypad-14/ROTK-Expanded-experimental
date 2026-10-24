#include "lib/summon.h"
#include "patches/ringwraith/ringwraith_kit.h"
#include "patches/skill_expansion/skill_expansion_elite.h"

#if HACK_skill_expansion

// Byte of the stock ghost record that holds the distance at which it starts an attack.
#define GHOST_REACH(ghost) (((u8 *)(ghost))[0x92])

// Pixels past its reach that a foe may drift before the thrall abandons the swing.
#define SWING_SLACK 16

static s32 pixelDistance(Actor *from, Actor *to)
{
    s32 dx = (to->coords.x - from->coords.x) >> FP16_16_SHIFT;
    s32 dy = (to->coords.y - from->coords.y) >> FP16_16_SHIFT;

    return ABS(dx) + ABS(dy);
}

/** Drop the swing and go back to hunting. */
static void recover(Actor *thrall, GhostState *state)
{
    thrall->actionState = state->target != NULL ? GHOST_ACTION_PURSUIT : GHOST_ACTION_IDLE;
    thrall->actionStateTimer = 0;
    thrall->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    thrall->field_0x64[0] |= 2;
}

static void retarget(Actor *thrall, GhostState *state)
{
    Actor *foe = NULL;
    Actor *best = NULL;
    s32 bestDistance = 0;

    while ((foe = findNextEnemy(thrall->parentActor, foe)) != NULL)
    {
        s32 distance = pixelDistance(thrall, foe);

        if (skillExpansion_isFoe(foe) && (best == NULL || distance < bestDistance))
        {
            best = foe;
            bestDistance = distance;
        }
    }
    state->target = best;
}

/**
 * Stock ghosts finish their swing even when the foe is already dead or has moved away, and so
 * stand in place hitting nothing. A thrall stops swinging as soon as its target is gone or out
 * of reach, then acquires new foes until its lifetime expires.
 */
void ringwraith_tickThrall(Actor *thrall)
{
    GhostState *state = ghostState(thrall);

    if (thrall->behaviorState != GHOST_BEHAVIOR_DESPAWNING && state->lifetime > 1)
    {
        bool32 swinging = thrall->actionState == GHOST_ACTION_ATTACK;
        bool32 lost = state->target == NULL || !skillExpansion_isFoe(state->target);

        if (lost)
        {
            retarget(thrall, state);
        }
        if (thrall->actionState == GHOST_ACTION_DEATH ||
            (swinging && ((thrall->flags.p & ACTOR_FLAG_ANIMATION_END) || lost ||
                          pixelDistance(thrall, state->target) > GHOST_REACH(thrall) + SWING_SLACK)))
        {
            recover(thrall, state);
        }
    }
    actor_npc_tickPassive(thrall);
}

#endif
