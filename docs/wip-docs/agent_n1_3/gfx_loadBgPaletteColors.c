#include "gfx.h"

void gfx_loadPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount, void *bank);

// Stand-in for the BG palette bank at 0x03001ac0.
#define g_BgPaletteBank ((void *)0x03001ac0)

/**
 * Copy raw colors into the BG palette buffer and palette RAM.
 *
 * @romaddress 0x08016cbc
 */
void gfx_loadBgPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount)
{
    gfx_loadPaletteColors(colors, firstColor, colorCount, g_BgPaletteBank);
}
