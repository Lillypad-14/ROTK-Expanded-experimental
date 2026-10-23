#include "gba_io.h"
#include "gfx.h"

/**
 * Current display video mode (DISPCNT bits 0-2).
 *
 * @romaddress 0x0801b078
 */
u32 gfx_getVideoMode(void)
{
    return REG_DISPCNT & 7;
}
