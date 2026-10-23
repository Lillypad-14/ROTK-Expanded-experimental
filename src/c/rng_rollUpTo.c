#include "rng.h"

/**
 * MT19937-stream roll in [0, n].
 *
 * @romaddress 0x0803a534
 */
u32 rng_rollUpTo(u16 n)
{
    u32 roll = rng_nextMT19937() & 0x7fff;

    roll *= n + 1;
    return (roll * 2) >> 16;
}
