#include "actor.h"
#include "memory.h"

/**
 * Take a free pool block, clear it, and prepend it to the active list.
 *
 * @return NULL when the pool is exhausted.
 *
 * @romaddress 0x08030518
 */
Actor *pool_alloc(Actor **freeList, ActorList *list, size_t size)
{
    Actor *actor;
    Actor *result;

    if (*freeList != NULL)
    {
        actor = pool_popFree(freeList);
        memory_memset(actor, 0, size);
        list_pushFront(list, actor);
        result = actor;
    }
    else
    {
        result = NULL;
    }
    return result;
}

/**
 * Unlink an actor and return its block to the pool's free list.
 *
 * @romaddress 0x0803054c
 */
void pool_free(Actor **freeList, ActorList *list, Actor *actor)
{
    list_remove(list, actor);
    list_pushFront((ActorList *)freeList, actor);
}
