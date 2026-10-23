#include "rng.h"

s32 rng_range(s32 lo, s32 hi)
{
    s32 roll = rng_nextMT19937() & 0x7fff;

    return lo + ((roll * (hi - lo + 1)) >> 15);
}
