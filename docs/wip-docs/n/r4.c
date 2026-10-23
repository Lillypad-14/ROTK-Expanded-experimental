#include "rng.h"

s16 rng_signedOffset(u16 n)
{
    u32 roll = rng_nextGameplay() & 0x7fff;
    s16 range = n;

    return ((roll * (range * 2 + 1)) >> 15) - range;
}
