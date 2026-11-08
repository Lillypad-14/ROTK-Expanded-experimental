#pragma once

#include "actor.h"
#include "actor/npc.h"
#include "lib/actor_ref.h"
#include "variables.h"

#define GHOST_NPC_KIND NPC_KIND_SIEGE_B
#define GHOST_BEHAVIOR_DESPAWNING 1
#define GHOST_ACTION_IDLE 2
#define GHOST_ACTION_ATTACK 3
#define GHOST_ACTION_PURSUIT 8
#define GHOST_ACTION_DEATH 0x3d

// Actor.as view of a stock ghost summon.
typedef struct GhostState
{
    // Enemy the ghost attacks.
    Actor *target;
    // Summoning player avatar.
    Actor *owner;
    // Frames left. Values <= 1 are reserved for expiry or consumption.
    u16 lifetime;
    u8 field_0x0a[7];
    // Damage per hit.
    u8 damage;
} GhostState;

// A ghost's melee reach in pixels.
#define HACKLIB_GHOST_REACH(ghost) (((u8 *)(ghost))[0x92])
// Pixels past its reach that a foe may drift before a ghost abandons the swing.
#define HACKLIB_GHOST_SWING_SLACK 16

static inline GhostState *ghostState(Actor *ghost)
{
    return (GhostState *)&ghost->as;
}

typedef bool32 (*HackLibSummonTargetFilter)(Actor *actor);
typedef Actor *(*FindNextEnemyFn)(Actor *playerActor, Actor *cursor);
#define findNextEnemy ((FindNextEnemyFn)(0x080013a8 | 1))

typedef Actor *(*SpawnNpcFn)(u8 kind, Vector2Fp16 *pos, Actor *owner, Actor *target, u16 lifetime);
#define spawnNpc ((SpawnNpcFn)(0x08035864 | 1))

/** Iterate usable owned ghosts, excluding summons already consumed or expiring. */
static inline Actor *hackLib_nextOwnedGhost(Actor *owner, Actor *cursor)
{
    Actor *actor = cursor != NULL ? cursor->next : g_ActorListMain.head;

    for (; actor != NULL; actor = actor->next)
    {
        if (actor->type == ACTOR_TYPE_NPC && actor->variant == GHOST_NPC_KIND &&
            actor->parentActor == owner && actor->behaviorState != GHOST_BEHAVIOR_DESPAWNING &&
            ghostState(actor)->lifetime > 1)
        {
            return actor;
        }
    }
    return NULL;
}

/**
 * Keeps a ghost near its owner: it fights foes near the owner, follows the owner when it has none
 * and is recalled to the owner's side when left far behind. A swing ends at once when its foe dies
 * or leaves reach. Call before actor_npc_tickPassive.
 */
void hackLib_escortGhost(Actor *ghost, HackLibSummonTargetFilter isTarget);

/** Opt-in persistent ghost behavior. Stock ticking owns lifetime and removal bookkeeping. */
static inline void hackLib_tickPersistentGhost(Actor *ghost, HackLibSummonTargetFilter isTarget,
                                               FindNextEnemyFn nextEnemy)
{
    GhostState *state = ghostState(ghost);

    if (ghost->behaviorState != GHOST_BEHAVIOR_DESPAWNING && state->lifetime > 1)
    {
        if (state->target == NULL || !isTarget(state->target))
        {
            Actor *foe = NULL;
            Actor *best = NULL;
            s32 bestDistance = 0;

            while ((foe = nextEnemy(ghost->parentActor, foe)) != NULL)
            {
                s32 dx = (foe->coords.x - ghost->coords.x) >> FP16_16_SHIFT;
                s32 dy = (foe->coords.y - ghost->coords.y) >> FP16_16_SHIFT;
                s32 distance = ABS(dx) + ABS(dy);

                if (isTarget(foe) && (best == NULL || distance < bestDistance))
                {
                    best = foe;
                    bestDistance = distance;
                }
            }
            state->target = best;
        }
        // A swing also ends when its foe dies, stops being a target or leaves reach: the stock ghost
        // finishes a swing standing still, and an ally wearing another enemy's looping attack clip
        // never sees the animation end, so it would swing in place forever.
        bool32 swinging = ghost->actionState == GHOST_ACTION_ATTACK;
        Actor *target = state->target;
        bool32 lost = target == NULL || !isTarget(target) || hackLib_actorCurrentHp(target) <= 0;

        if (swinging && !lost)
        {
            s32 dx = (target->coords.x - ghost->coords.x) >> FP16_16_SHIFT;
            s32 dy = (target->coords.y - ghost->coords.y) >> FP16_16_SHIFT;

            lost = ABS(dx) + ABS(dy) > HACKLIB_GHOST_REACH(ghost) + HACKLIB_GHOST_SWING_SLACK;
        }
        if (ghost->actionState == GHOST_ACTION_DEATH ||
            (swinging && ((ghost->flags.p & ACTOR_FLAG_ANIMATION_END) || lost)))
        {
            if (lost)
            {
                state->target = NULL;
            }
            ghost->actionState = state->target != NULL ? GHOST_ACTION_PURSUIT : GHOST_ACTION_IDLE;
            ghost->actionStateTimer = 0;
            ghost->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
            ghost->field_0x64[0] |= 2;
        }
    }
    actor_npc_tickPassive(ghost);
}
