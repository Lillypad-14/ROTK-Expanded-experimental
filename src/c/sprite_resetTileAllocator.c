#include "gba.h"
#include "libc.h"
#include "sprite.h"
#include "variables.h"

/**
 * Reset the object tile allocator bitmaps.
 *
 * @romaddress 0x08049a54
 */
void sprite_resetTileAllocator(void)
{
    bios_div(g_ObjTileWorldHighWater, 8);
    g_ObjTileWorldHighWater = 0;
    memset(g_ObjTileBitmap, 0, 0x80);
    memset(g_ObjTileDeferredBitmap, 0xff, 0x80);
}
