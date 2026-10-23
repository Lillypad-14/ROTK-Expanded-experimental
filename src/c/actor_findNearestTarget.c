#include "actor.h"
#include "math.h"
#include "player.h"
#include "variables.h"

/**
 * Find the player actor closest to self, writing its distance to outDistance (9999 when it
 * is not targetable).
 *
 * @romaddress 0x08008b74
 */
Actor *actor_unknown08008b74(Actor *self, u32 *outDistance)
{
    u8 unitType = self->unitType;
    u8 targetsHidden = 0;
    Actor *best;
    Actor *candidate;
    Vector2Int selfPos;
    Vector2Int bestPos;
    u32 bestDist;
    u32 dist;
    u8 i;

    if (unitType == 0x3a || unitType == 0x44 || unitType == 0x41)
        targetsHidden = 1;

    best = g_PlayerGlobals.players[0].ownerActor;
    bestPos = *(Vector2Int *)&best->coords;
    selfPos = *(Vector2Int *)&self->coords;
    bestDist = math_vector2_distance(selfPos, bestPos);
    bestDist >>= 16;

    if (g_PlayerGlobals.activePlayerCount > 1)
    {
        for (i = 1; i < g_PlayerGlobals.activePlayerCount; i++)
        {
            candidate = g_PlayerGlobals.players[i].ownerActor;
            if (!(candidate->flags.p & ACTOR_FLAG_23) || (targetsHidden && (candidate->as.combat.actionStateFlags & 8)))
            {
                dist = math_vector2_distance(selfPos, *(Vector2Int *)&candidate->coords) >> 16;
                if (dist < bestDist)
                {
                    best = candidate;
                    bestDist = dist;
                }
            }
        }
    }
    else if (!(self->flags.p & ACTOR_FLAG_NO_FLINCH) && unitType != 0x3a)
    {
        for (i = 0; i < 2; i++)
        {
            candidate = g_CompanionActors[i];
            if (candidate != NULL && !(candidate->flags.p & ACTOR_FLAG_23))
            {
                dist = math_vector2_distance(selfPos, *(Vector2Int *)&candidate->coords) >> 16;
                if (dist < bestDist)
                {
                    best = candidate;
                    bestDist = dist;
                }
            }
        }
    }

    if (!(best->flags.p & ACTOR_FLAG_23) || (targetsHidden && (best->as.combat.actionStateFlags & 8)))
        *outDistance = bestDist;
    else
        *outDistance = 9999;
    return best;
}
