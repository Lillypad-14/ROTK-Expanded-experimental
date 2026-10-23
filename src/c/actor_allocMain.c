#include "sprite.h"
#include "variables.h"

/**
 * Allocate and initialize an actor from the main gameplay pool.
 *
 * @return NULL when the main pool is exhausted.
 *
 * @romaddress 0x0800169c
 */
Actor *actor_allocMain(void)
{
    Actor *actor = pool_alloc(&g_ActorPoolMainFreeList, &g_ActorListMain, sizeof(Actor));

    if (actor != NULL)
    {
        if (g_ActorListMain.tail == NULL)
        {
            g_ActorListMain.tail = actor;
        }
        actor->type = ACTOR_TYPE_STORAGE_MAX;
        actor->tileNumber = 0xffff;
        actor->listId = ACTOR_LIST_MAIN;
        actor->sprite.scaleFramesLeft = 0;
        sprite_setOamMatrixIndex(&actor->oam, 0);
    }
    return actor;
}
