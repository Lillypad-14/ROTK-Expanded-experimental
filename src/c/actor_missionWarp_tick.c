#include "actor.h"
#include "actor/missionWarp.h"
#include "math.h"
#include "player.h"
#include "variables.h"

/**
 * Per-frame update: tracks the nearest player distance and raises the HUD text flag
 * while a player is within range.
 *
 * @romaddress 0x080326ac
 */
void actor_missionWarp_tick(Actor *actor)
{
    u32 i;

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
