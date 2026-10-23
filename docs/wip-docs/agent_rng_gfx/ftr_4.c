#include "gfx.h"
#include "variables.h"

void gfx_fillTilemapRect(u16 *tilemap, s32 col, s32 row, s32 width, s32 height, u16 tile)
{
    s32 y; s32 x; u16 *dst;
    for (y = 0; y < height; y++)
    {
        dst = tilemap + (row + y) * 32 + col;
        if (width > 0) {
        x = width;
        do { *dst++ = tile; } while (--x);
        }
    }
}
