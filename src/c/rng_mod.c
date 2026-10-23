#include "rng.h"

/**
 * Gameplay-stream roll in [0, n].
 *
 * @romaddress 0x0803a510
 */
s32 rng_mod(u16 n)
{
    u32 roll = rng_nextGameplay() & 0x7fff;

    roll *= n + 1;
    return (roll * 2) >> 16;
}
