#include "rng.h"

/**
 * As rng_rollRange, but drawn from the MT19937 stream instead of the gameplay stream.
 *
 * @romaddress 0x0803a4e8
 */
s32 rng_range(s32 lo, s32 hi)
{
    s32 roll = rng_nextMT19937() & 0x7fff;

    return lo + ((roll * (hi - lo + 1)) >> 15);
}
