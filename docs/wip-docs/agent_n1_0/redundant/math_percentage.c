#include "math.h"
#include "libc.h"

/**
 * Compute value as a percentage of total.
 *
 * @romaddress 0x08032a70
 */
s32 math_percentage(s32 value, s32 total)
{
    return bios_div(value * 100, total);
}
