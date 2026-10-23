#include "gfx.h"
#include "variables.h"

/**
 * Fill a rectangle of a BG layer's tilemap with one tile.
 *
 * @romaddress 0x08012660
 */
void gfx_fillBgRect(s32 layer, s32 col, s32 row, s32 width, s32 height, u16 tile)
{
    gfx_fillTilemapRect(g_BgLayers[layer].tilemap, col, row, width, height, tile);
}

/**
 * Fill a rectangle of a 32-entry-wide tilemap with one tile.
 *
 * @romaddress 0x0801268c
 */
void gfx_fillTilemapRect(u16 *tilemap, s32 col, s32 row, s32 width, s32 height, u16 tile)
{
    s32 x;
    u16 *dst;
    s32 y;

    for (y = 0; y < height; y++)
    {
        dst = tilemap + (row + y) * 32 + col;
        for (x = width; x > 0; x--)
        {
            *dst++ = tile;
        }
    }
}
