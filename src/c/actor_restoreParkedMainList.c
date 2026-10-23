#include "actor.h"
#include "libc.h"
#include "sprite.h"
#include "variables.h"

/**
 * Resume the parked world and invalidate cached sprite graphics.
 *
 * @romaddress 0x08001838
 */
void actor_restoreParkedMainList(void)
{
    g_ActorListMain.head = g_ActorListParked.head;
    g_ActorListMain.tail = g_ActorListParked.tail;
    g_ActorListParked.head = NULL;
    g_ActorListParked.tail = NULL;
    actor_clearFlagsAll(ACTOR_FLAG_TICK_SUPPRESSED);
    memset(g_SpriteCellCache, 0, SPRITE_CELL_CACHE_SIZE);
}
