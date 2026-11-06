#include "lib/summon.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"

// Distances are in pixels, measured as |dx| + |dy|.
// Foes farther than this from the owner are left alone.
#define AGGRO_RANGE 140
// A ghost chasing a foe gives up once the foe is this far from the owner.
#define LEASH_RANGE 190
// A ghost without a foe starts following the owner beyond this distance.
#define FOLLOW_START 90
// A following ghost stops this far past its own reach, so it never swings at the owner.
#define FOLLOW_STOP_MARGIN 28
// A ghost this far from the owner is brought back to its side. The stock ghost drops any target
// more than 160 pixels away, so a ghost cannot walk back from much farther than this.
#define RECALL_RANGE 150
// Where a recalled ghost lands, to the side of the owner.
#define RECALL_OFFSET 28

static s32 pixelDistance(Actor *from, Actor *to)
{
    s32 dx = (to->coords.x - from->coords.x) >> FP16_16_SHIFT;
    s32 dy = (to->coords.y - from->coords.y) >> FP16_16_SHIFT;

    return ABS(dx) + ABS(dy);
}

/** Drop the swing and go back to hunting. */
static void recover(Actor *ghost, GhostState *state)
{
    ghost->actionState = state->target != NULL ? GHOST_ACTION_PURSUIT : GHOST_ACTION_IDLE;
    ghost->actionStateTimer = 0;
    ghost->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    ghost->field_0x64[0] |= 2;
}

/** The nearest foe within AGGRO_RANGE of the owner. */
static Actor *nearestFoe(Actor *ghost, Actor *owner, HackLibSummonTargetFilter isTarget)
{
    Actor *foe = NULL;
    Actor *best = NULL;
    s32 bestDistance = 0;

    while ((foe = findNextEnemy(owner, foe)) != NULL)
    {
        s32 distance = pixelDistance(ghost, foe);

        if (isTarget(foe) && foe->currentHp > 0 && pixelDistance(owner, foe) <= AGGRO_RANGE &&
            (best == NULL || distance < bestDistance))
        {
            best = foe;
            bestDistance = distance;
        }
    }
    return best;
}

/** Put the ghost beside the owner when it has fallen far behind. */
static void recall(Actor *ghost, Actor *owner)
{
    Vector2Fp16 preferred = owner->coords;
    Vector2Fp16 landing;

    preferred.x += (owner->coords.x < ghost->coords.x ? 1 : -1) * (RECALL_OFFSET << 16);
    if (!hackLib_findLanding(owner, &preferred, &landing))
    {
        return;
    }
    hackLib_fxBurst(ghost, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    ghost->renderCoords.x += landing.x - ghost->coords.x;
    ghost->renderCoords.y += landing.y - ghost->coords.y;
    ghost->coords = landing;
    ghost->velocity.x = ghost->velocity.y = 0;
}

void hackLib_escortGhost(Actor *ghost, HackLibSummonTargetFilter isTarget)
{
    GhostState *state = ghostState(ghost);
    Actor *owner = ghost->parentActor;

    if (ghost->behaviorState == GHOST_BEHAVIOR_DESPAWNING || state->lifetime <= 1 || owner == NULL)
    {
        return;
    }

    bool32 swinging = ghost->actionState == GHOST_ACTION_ATTACK;
    bool32 following = state->target == owner;
    bool32 lost = state->target == NULL || following || !isTarget(state->target) ||
                  state->target->currentHp <= 0 || pixelDistance(owner, state->target) > LEASH_RANGE;
    s32 ownerDistance = pixelDistance(ghost, owner);

    if (lost)
    {
        state->target = nearestFoe(ghost, owner, isTarget);
    }
    if (state->target == NULL)
    {
        if (ownerDistance > RECALL_RANGE)
        {
            recall(ghost, owner);
        }
        else if (ownerDistance > FOLLOW_START ||
                 (following && ownerDistance > HACKLIB_GHOST_REACH(ghost) + FOLLOW_STOP_MARGIN))
        {
            state->target = owner;
        }
    }
    if (ghost->actionState == GHOST_ACTION_DEATH ||
        (swinging && ((ghost->flags.p & ACTOR_FLAG_ANIMATION_END) || lost ||
                      pixelDistance(ghost, state->target) >
                          HACKLIB_GHOST_REACH(ghost) + HACKLIB_GHOST_SWING_SLACK)) ||
        (state->target == owner && ghost->actionState != GHOST_ACTION_PURSUIT) ||
        (state->target == NULL && ghost->actionState == GHOST_ACTION_PURSUIT))
    {
        recover(ghost, state);
    }
}
