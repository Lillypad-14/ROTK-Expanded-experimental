#include "gba_io.h"
#include "gfx.h"

u32 gfx_getVideoMode(void)
{
    return REG_DISPCNT & 7;
}
