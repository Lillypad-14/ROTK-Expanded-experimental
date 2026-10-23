#include "actor.h"
#include "variables.h"

/**
 * Set the selected flags on every actor in the main list.
 *
 * @romaddress 0x08001874
 */
void actor_setFlagsAll(u32 mask)
{
    Actor *actor;

    for (actor = g_ActorListMain.head; actor != NULL; actor = actor->next)
    {
        actor->flags.p |= mask;
    }
}

/**
 * Clear the selected flags on every actor in the main list.
 *
 * @romaddress 0x08001898
 */
void actor_clearFlagsAll(u32 mask)
{
    Actor *actor;

    for (actor = g_ActorListMain.head; actor != NULL; actor = actor->next)
    {
        actor->flags.p &= ~mask;
    }
}
