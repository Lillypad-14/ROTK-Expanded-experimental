#include "gfx.h"
/**
 * Load a BG asset onto a layer through the alternate loader.
 *
 * @romaddress 0x080127d0
 */
void *gfx_unknown080127d0(u32 layer, const void *asset, u16 arg3, u16 arg4, u32 arg5, u32 arg6)
{
    void *data = gfx_unknown08011c8c(layer, asset, arg3);

    if (data != NULL)
    {
        gfx_unknown08011d78(layer, data, arg3, arg4, arg5, arg6);
    }
    return data;
}
