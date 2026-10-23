#include "gba_io.h"
#include "gfx.h"

/**
 * Write the blend control register: arg1 selects first-target layers, arg2 second-target layers,
 * and the alpha blend effect is enabled.
 *
 * @romaddress 0x0803dee8
 */
void gfx_setBlendControl(u16 arg1, u16 arg2)
{
    REG_BLDCNT = arg1 | 0x40 | (arg2 << 8);
}
