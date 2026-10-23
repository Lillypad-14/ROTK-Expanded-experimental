#include "sprite.h"

/**
 * Mark the tiles at tileNumber covering gfxSize bytes in the deferred free bitmap.
 *
 * @romaddress 0x08049c28
 */
void sprite_freeObjTilesDeferred(u16 tileNumber, u32 gfxSize)
{
    sprite_setObjTileBitsDeferred(tileNumber, (gfxSize << 11) >> 16);
}
