#include "actor.h"
#include "math.h"
#include "variables.h"

#define ITERATE_FLAGS_MASK (ACTOR_FLAG_10 | ACTOR_FLAG_18 | ACTOR_FLAG_28)

/**
 * Iterate the on-screen main-list actors within radius (16.16 fixed world units) of center. Pass
 * NULL as cursor to start, then the previous return each call; returns NULL when the sweep is done.
 *
 * @romaddress 0x08001fc8
 */
Actor *actor_iterateInRadius(Actor *center, Actor *cursor, s32 radius)
{
    Actor *actor;
    Actor *found = NULL;

    if (cursor == NULL)
    {
        actor = g_ActorListMain.head;
    }
    else
    {
        actor = cursor->next;
    }
    if (actor == NULL)
    {
        return NULL;
    }
    do
    {
        if ((actor->flags.p & ACTOR_FLAG_ON_SCREEN) && actor->actionState != ACTOR_STATE_UNKNOWN_11 &&
            actor->actionState != ACTOR_STATE_UNKNOWN_15 &&
            actor->actionState != ACTOR_STATE_UNKNOWN_04 &&
            math_vector2_distance(*(Vector2Int *)&center->renderCoords,
                              *(Vector2Int *)&actor->renderCoords) <= (u32)radius &&
            (actor->flags.p & ITERATE_FLAGS_MASK))
        {
            found = actor;
            break;
        }
        actor = actor->next;
    } while (actor != NULL);
    return found;
}
