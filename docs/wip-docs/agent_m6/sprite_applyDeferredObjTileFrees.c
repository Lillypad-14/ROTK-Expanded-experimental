#include "sprite.h"
#include "variables.h"

/**
 * Release the deferred object tile frees into the allocator bitmap and reset the deferred bitmap.
 *
 * @romaddress 0x08049984
 */
void sprite_applyDeferredObjTileFrees(void)
{
    u32 i = 0;
    u32 fill = 0xffffffff;
    u32 *deferred = (u32 *)g_ObjTileDeferredBitmap;
    u32 *bitmap = (u32 *)g_ObjTileBitmap;

    for (; i < 0x20; i += 8)
    {
        bitmap[0] &= deferred[0];
        bitmap[1] &= deferred[1];
        bitmap[2] &= deferred[2];
        bitmap[3] &= deferred[3];
        bitmap[4] &= deferred[4];
        bitmap[5] &= deferred[5];
        bitmap[6] &= deferred[6];
        bitmap[7] &= deferred[7];
        deferred[0] = fill;
        deferred[1] = fill;
        deferred[2] = fill;
        deferred[3] = fill;
        deferred[4] = fill;
        deferred[5] = fill;
        deferred[6] = fill;
        deferred[7] = fill;
        deferred += 8;
        bitmap += 8;
    }
}
