#include "actor.h"

/**
 * Remove the first free block from a pool's linked list.
 *
 * @return The removed block, or NULL when the list is empty.
 *
 * @romaddress 0x08030568
 */
Actor *pool_popFree(Actor **freeList)
{
    Actor *actor = *freeList;

    if (actor != NULL)
    {
        *freeList = actor->next;
        actor->next->previous = NULL;
        actor->next = NULL;
    }
    return actor;
}

/**
 * Prepend an actor to a list, preserving its backward links.
 *
 * @romaddress 0x08030580
 */
void list_pushFront(ActorList *list, Actor *actor)
{
    Actor *head = list->head;

    actor->next = head;
    actor->previous = NULL;
    if (head != NULL)
    {
        head->previous = actor;
    }
    list->head = actor;
}

/**
 * Unlink an actor and clear its list links.
 *
 * @romaddress 0x08030598
 */
void list_remove(ActorList *list, Actor *actor)
{
    if (list->head == actor)
    {
        list->head = actor->next;
    }
    if (actor->next != NULL)
    {
        actor->next->previous = actor->previous;
    }
    if (actor->previous != NULL)
    {
        actor->previous->next = actor->next;
    }
    actor->next = NULL;
    actor->previous = NULL;
}
