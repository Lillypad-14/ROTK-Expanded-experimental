#define math_sinFp8 hdr_dummy
#include "math.h"
#undef math_sinFp8

/**
 * Sine of an angle in 8.8 fixed point.
 *
 * @romaddress 0x08032818
 */
fp8_8 math_sinFp8(Angle8 angle)
{
    return math_sinFp16(angle) >> 8;
}
