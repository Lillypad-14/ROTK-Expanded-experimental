#include "gfx.h"
#include "variables.h"

/**
 * Copy raw colors into the BG palette buffer and palette RAM.
 *
 * @romaddress 0x08016cbc
 */
void gfx_loadBgPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount)
{
    gfx_loadPaletteColors(colors, firstColor, colorCount, &g_PaletteBanks[0]);
}
