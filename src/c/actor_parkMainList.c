#include "actor.h"
#include "variables.h"

/**
 * Suppress main-list updates and park the world while another scene runs.
 *
 * @romaddress 0x08001810
 */
void actor_parkMainList(void)
{
    actor_setFlagsAll(ACTOR_FLAG_TICK_SUPPRESSED);
    g_ActorListParked.head = g_ActorListMain.head;
    g_ActorListParked.tail = g_ActorListMain.tail;
    g_ActorListMain.head = NULL;
    g_ActorListMain.tail = NULL;
}
