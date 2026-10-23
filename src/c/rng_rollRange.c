#include "rng.h"

/**
 * Roll an integer in [lo, hi] from the gameplay random stream.
 *
 * @romaddress 0x0803a4bc
 */
s32 rng_rollRange(s32 lo, s32 hi)
{
    s32 roll;

    if (lo == hi)
    {
        return lo;
    }
    roll = rng_nextGameplay() & 0x7fff;
    return lo + (((hi - lo + 1) * roll) >> 15);
}
