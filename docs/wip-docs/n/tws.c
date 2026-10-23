#include "gfx.h"
#include "terrain.h"
#include "variables.h"

void terrain_getWorldSize(Vector2Int *size)
{
    if ((u16)gfx_getVideoMode() <= 2)
    {
        size->x = g_WorldWidthTiles << 3;
        size->y = g_WorldHeightTiles << 3;
    }
    else
    {
        size->x = 240;
        size->y = 160;
    }
}
