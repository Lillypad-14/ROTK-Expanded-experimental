#define gfx_loadObjPaletteColors hdr_dummy
#include "gfx.h"
#undef gfx_loadObjPaletteColors

void gfx_loadPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount, void *bank);

/**
 * Load colors into the OBJ palette.
 *
 * @romaddress 0x08016ccc
 */
void gfx_loadObjPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount)
{
    gfx_loadPaletteColors(colors, firstColor, colorCount, (void *)0x03001ac8);
}
