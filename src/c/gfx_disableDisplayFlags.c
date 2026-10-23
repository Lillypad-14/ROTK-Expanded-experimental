#include "gba_io.h"
#include "gfx.h"

/**
 * Clear bits in the display control register.
 *
 * @romaddress 0x080124fc
 */
void gfx_disableDisplayFlags(u16 mask)
{
    REG_DISPCNT &= ~mask;
}
