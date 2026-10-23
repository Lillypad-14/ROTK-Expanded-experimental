#include "rng.h"

/**
 * True with a probability of about percent out of 100, drawn from the gameplay stream.
 *
 * @romaddress 0x0803a5b0
 */
bool rng_rollPercent(u16 percent)
{
    u32 roll = rng_nextGameplay() & 0x7fff;

    if (percent < (roll * 101) >> 15)
    {
        return FALSE;
    }
    return TRUE;
}
