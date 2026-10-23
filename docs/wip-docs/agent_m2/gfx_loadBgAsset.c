#include "gfx.h"

/* Needs config rows (see report):
 * 0x08011bc0 thumb gfx_unknown08011bc0
 * 0x08011d78 thumb gfx_unknown08011d78
 */
void *gfx_unknown08011bc0(u32 layer, const void *asset, u16 arg3, u16 arg4);
void gfx_unknown08011d78(u32 layer, void *loaded, u16 arg3, u16 arg4, u32 arg5, u32 arg6);

/**
 * Load a background asset onto a layer.
 *
 * @romaddress 0x08012794
 */
void *gfx_loadBgAsset(u32 layer, const void *asset, u16 arg3, u16 arg4, u32 arg5, u32 arg6)
{
    void *loaded = gfx_unknown08011bc0(layer, asset, arg3, arg4);

    if (loaded != NULL)
    {
        gfx_unknown08011d78(layer, loaded, arg3, arg4, arg5, arg6);
    }
    return loaded;
}
