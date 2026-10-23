#include "sprite.h"

/**
 * Free the tiles at tileNumber covering gfxSize bytes at once.
 *
 * @romaddress 0x08049c14
 */
void sprite_freeObjTiles(u16 tileNumber, u32 gfxSize)
{
    sprite_clearObjTileBits(tileNumber, (gfxSize << 11) >> 16);
}
