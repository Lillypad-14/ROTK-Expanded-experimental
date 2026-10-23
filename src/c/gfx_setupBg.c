#include "gfx.h"

/**
 * Configure a background layer's control register and enable it.
 *
 * @romaddress 0x080123e0
 */
void gfx_setupBg(u32 layer, u32 bgcnt)
{
    gfx_setBgControl(layer, bgcnt);
    gfx_enableDisplayFlags((0x1000000 << layer) >> 16);
}
