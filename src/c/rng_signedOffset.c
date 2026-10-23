#include "rng.h"

/**
 * Roll a gameplay-stream offset in [-n, n].
 *
 * @romaddress 0x0803a558
 */
s16 rng_signedOffset(u16 n)
{
    u32 roll = rng_nextGameplay() & 0x7fff;
    s32 r = (s16)n;

    return ((roll * (r * 2 + 1)) >> 15) - r;
}
