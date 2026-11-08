// Mission warps for low hero bodies. A warp fires when its trigger box touches each player's
// hurtbox, and an enemy body's hurtbox can be much lower than a stock hero's (the Warg's reaches 20
// pixels above its feet, a stock hero's about 35), so it stopped short of a small doorway warp.
#include "actor.h"
#include "actor/missionWarp.h"
#include "math.h"
#include "player.h"
#include "variables.h"

#if PC_EDITION

// Height a mission warp's trigger box grows to, centered on its own.
#define HERO_DOOR_MIN_HEIGHT 24

/**
 * actor_missionWarp_tick with the warp's trigger box at least HERO_DOOR_MIN_HEIGHT tall.
 *
 * REPLACES (whole function): actor_missionWarp_tick.
 */
void heroDoors_missionWarpTick(Actor *actor)
{
    struct ActorTriggerBox *box = &actor->triggerBoxes[0];
    s32 grow = (HERO_DOOR_MIN_HEIGHT - (box->yMax - box->yMin) + 1) / 2;
    u32 i;

    if (grow > 0)
    {
        box->yMin -= grow;
        box->yMax += grow;
    }
    if (actor->behaviorState == 0)
    {
        actor->as.missionWarp.playersInside = 0;
        for (i = 0; i < g_PlayerGlobals.activePlayerCount; i++)
        {
            Actor *owner = PLAYER(i).ownerActor;
            s32 distance = math_vector2_distance(*(Vector2Int *)&actor->coords, *(Vector2Int *)&owner->coords);

            distance >>= 16;
            if (distance < PLAYER(i).nearestWarpDistance)
            {
                PLAYER(i).nearestWarpDistance = distance;
            }
            if (PLAYER(i).nearestWarpDistance <= 0x9f)
            {
                g_PlayerHuds[i].flags.p |= 1 << 13;
            }
            else
            {
                g_PlayerHuds[i].flags.p &= ~(1 << 13);
            }
        }
    }
}

#endif
