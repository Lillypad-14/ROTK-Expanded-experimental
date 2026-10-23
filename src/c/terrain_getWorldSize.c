#include "gfx.h"
#include "terrain.h"
#include "variables.h"

/**
 * Current world dimensions in pixels. Bitmap video modes use the full screen.
 *
 * @romaddress 0x0803f2a8
 */
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
