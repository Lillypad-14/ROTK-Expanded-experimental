#define math_multiplyFp8 hdr_dummy
#include "math.h"
#undef math_multiplyFp8

/**
 * Multiply two 8.8 fixed point values.
 *
 * @romaddress 0x08032840
 */
fp8_8 math_multiplyFp8(fp8_8 a, fp8_8 b)
{
    return (a * b) / 256;
}
