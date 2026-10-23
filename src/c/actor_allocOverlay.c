#include "sprite.h"
#include "variables.h"

/**
 * Allocate and initialize an actor for an overlay menu.
 *
 * @return NULL when the overlay pool is exhausted.
 *
 * @romaddress 0x08001918
 */
Actor *actor_allocOverlay(void)
{
    Actor *actor = pool_alloc(&g_ActorPoolOverlayFreeList, &g_ActorListOverlay, sizeof(Actor));

    if (g_ActorListOverlay.tail == NULL)
    {
        g_ActorListOverlay.tail = actor;
    }
    if (actor != NULL)
    {
        actor->type = ACTOR_TYPE_STORAGE_MAX;
        actor->tileNumber = 0xffff;
        actor->listId = ACTOR_LIST_OVERLAY;
    }
    return actor;
}
