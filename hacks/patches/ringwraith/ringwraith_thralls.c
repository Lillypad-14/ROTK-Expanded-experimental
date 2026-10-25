#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "lib/summon.h"
#include "patches/ringwraith/ringwraith_kit.h"
#include "patches/skill_expansion/skill_expansion_elite.h"

#if HACK_skill_expansion

// Byte of the stock ghost record that holds the distance at which it starts an attack.
#define GHOST_REACH(ghost) (((u8 *)(ghost))[0x92])

// Pixels past its reach that a foe may drift before the thrall abandons the swing.
#define SWING_SLACK 16

// Distances are in pixels, measured as |dx| + |dy|.
// Foes farther than this from the wraith are left alone.
#define AGGRO_RANGE 140
// A thrall chasing a foe gives up once the foe is this far from the wraith.
#define LEASH_RANGE 190
// A thrall without a foe starts following the wraith beyond this distance.
#define FOLLOW_START 90
// A following thrall stops this far past its own reach, so it never swings at the wraith.
#define FOLLOW_STOP_MARGIN 28
// A thrall this far from the wraith is brought back to its side. The stock ghost drops any target
// more than 160 pixels away, so a thrall cannot walk back from much farther than this.
#define RECALL_RANGE 150
// Where a recalled thrall lands, to the side of the wraith.
#define RECALL_OFFSET 28

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

/** The nearest foe within AGGRO_RANGE of the wraith. */
static Actor *nearestFoe(Actor *thrall, Actor *owner)
{
    Actor *foe = NULL;
    Actor *best = NULL;
    s32 bestDistance = 0;

    while ((foe = findNextEnemy(owner, foe)) != NULL)
    {
        s32 distance = pixelDistance(thrall, foe);

        if (skillExpansion_isFoe(foe) && pixelDistance(owner, foe) <= AGGRO_RANGE &&
            (best == NULL || distance < bestDistance))
        {
            best = foe;
            bestDistance = distance;
        }
    }
    return best;
}

/** Put the thrall beside the wraith when it has fallen far behind. */
static void recall(Actor *thrall, Actor *owner)
{
    Vector2Fp16 preferred = owner->coords;
    Vector2Fp16 landing;

    preferred.x += (owner->coords.x < thrall->coords.x ? 1 : -1) * (RECALL_OFFSET << 16);
    if (!hackLib_findLanding(owner, &preferred, &landing))
    {
        return;
    }
    hackLib_fxBurst(thrall, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    thrall->renderCoords.x += landing.x - thrall->coords.x;
    thrall->renderCoords.y += landing.y - thrall->coords.y;
    thrall->coords = landing;
    thrall->velocity.x = thrall->velocity.y = 0;
}

/**
 * Thralls fight foes near the wraith and otherwise stay at its side. A foe that dies or leaves
 * reach ends the swing at once, because the stock ghost finishes its swing standing still. With
 * no foe to fight, a thrall follows the wraith while it is more than FOLLOW_START away, and a
 * thrall left far behind is recalled.
 */
void ringwraith_tickThrall(Actor *thrall)
{
    GhostState *state = ghostState(thrall);
    Actor *owner = thrall->parentActor;

    if (thrall->behaviorState != GHOST_BEHAVIOR_DESPAWNING && state->lifetime > 1 && owner != NULL)
    {
        bool32 swinging = thrall->actionState == GHOST_ACTION_ATTACK;
        bool32 following = state->target == owner;
        bool32 lost = state->target == NULL || following || !skillExpansion_isFoe(state->target) ||
                      pixelDistance(owner, state->target) > LEASH_RANGE;
        s32 ownerDistance = pixelDistance(thrall, owner);

        if (lost)
        {
            state->target = nearestFoe(thrall, owner);
        }
        if (state->target == NULL)
        {
            if (ownerDistance > RECALL_RANGE)
            {
                recall(thrall, owner);
            }
            else if (ownerDistance > FOLLOW_START ||
                     (following && ownerDistance > GHOST_REACH(thrall) + FOLLOW_STOP_MARGIN))
            {
                state->target = owner;
            }
        }
        if (thrall->actionState == GHOST_ACTION_DEATH ||
            (swinging && ((thrall->flags.p & ACTOR_FLAG_ANIMATION_END) || lost ||
                          pixelDistance(thrall, state->target) > GHOST_REACH(thrall) + SWING_SLACK)) ||
            (state->target == owner && thrall->actionState != GHOST_ACTION_PURSUIT) ||
            (state->target == NULL && thrall->actionState == GHOST_ACTION_PURSUIT))
        {
            recover(thrall, state);
        }
    }
    actor_npc_tickPassive(thrall);
}

#endif
