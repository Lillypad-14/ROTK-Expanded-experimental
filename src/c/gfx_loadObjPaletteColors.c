#include "gfx.h"
#include "variables.h"

/**
 * Load colors into the OBJ palette.
 *
 * @romaddress 0x08016ccc
 */
void gfx_loadObjPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount)
{
    gfx_loadPaletteColors(colors, firstColor, colorCount, &g_PaletteBanks[1]);
}
