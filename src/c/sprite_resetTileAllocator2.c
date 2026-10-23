#include "libc.h"
#include "sprite.h"
#include "variables.h"

/**
 * Reset the object tile allocator bitmaps.
 *
 * @romaddress 0x08049a8c
 */
void sprite_resetTileAllocator2(void)
{
    g_ObjTileWorldHighWater = 0;
    memset(g_ObjTileBitmap, 0, 0x80);
    memset(g_ObjTileDeferredBitmap, 0xff, 0x80);
}
