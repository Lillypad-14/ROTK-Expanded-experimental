#include "gfx.h"

/**
 * Hide a background layer by clearing its DISPCNT enable bit.
 *
 * @romaddress 0x08012458
 */
void gfx_disableBg(u32 layer)
{
    gfx_disableDisplayFlags((0x100 << layer));
}
