#include "libc.h"
#include "sprite.h"
#include "variables.h"

/**
 * Reset the object tile allocator for a display mode.
 *
 * @romaddress 0x080499f8
 */
void sprite_resetTileAllocatorForMode(u8 displayMode)
{
    g_ObjTileWorldHighWater = 0;
    if ((u8)(displayMode - 3) < 3) {
        memset(g_ObjTileBitmap, 0xff, 0x40);
        memset(g_ObjTileBitmap + 0x40, 0, 0x40);
    } else {
        memset(g_ObjTileBitmap, 0, 0x80);
        memset(g_ObjTileDeferredBitmap, 0xff, 0x80);
    }
}
