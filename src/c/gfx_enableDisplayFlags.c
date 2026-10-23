#include "gba_io.h"
#include "gfx.h"

/**
 * Set bits in the display control register.
 *
 * @romaddress 0x0801250c
 */
void gfx_enableDisplayFlags(u16 mask)
{
    REG_DISPCNT |= mask;
}
