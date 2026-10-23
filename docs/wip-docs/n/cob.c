#include "sprite.h"
#include "variables.h"

void sprite_clearObjTileBits(u16 tileNumber, u16 tileCount)
{
    u32 tile;
    u32 end;
    u32 bit;
    u32 mask;
    u32 index;

    if (tileNumber == 0xffff)
    {
        return;
    }
    end = tileNumber + tileCount;
    for (tile = tileNumber; tile < end;)
    {
        index = tile >> 3;
        bit = tile & 7;
        if (bit == 0 && end - tile > 7)
        {
            mask = 0xff;
            tile += 8;
        }
        else
        {
            mask = 1 << bit;
            tile += 1;
        }
        g_ObjTileBitmap[index] &= ~mask;
    }
}
