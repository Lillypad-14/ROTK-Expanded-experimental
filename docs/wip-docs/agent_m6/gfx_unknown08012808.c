#define gfx_unknown08012808 gfx_unknown08012808_hdr
#define gfx_loadPalette gfx_loadPalette_hdr
#include "gfx.h"
#undef gfx_loadPalette
#undef gfx_unknown08012808

// loadPalette returns the byte size of the palette block it consumed.
u32 gfx_loadPalette(const void *paletteAsset, u32 paletteNumber);

/**
 * Load a BG asset's palette and, when its flags select a data block, pass that block on to
 * the layer loader.
 *
 * @return the data block pointer, or NULL when the asset has none
 *
 * @romaddress 0x08012808
 */
const void *gfx_unknown08012808(u32 layer, const u8 *asset, u16 arg3, u16 arg4, u32 arg5, u32 arg6)
{
    const u8 *block = NULL;
    const u8 *next = asset + 2;

    next += gfx_loadPalette(asset, arg4);
    if (asset[0] & 0x10)
    {
        block = next;
    }
    if (block != NULL)
    {
        gfx_unknown08011d78(layer, (void *)block, arg3, arg4, arg5, arg6);
    }
    return block;
}
