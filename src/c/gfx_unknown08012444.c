#include "gfx.h"

/**
 * Show a background layer by setting its DISPCNT enable bit.
 *
 * @romaddress 0x08012444
 */
void gfx_unknown08012444(s32 layer)
{
    gfx_enableDisplayFlags((0x100 << layer));
}
