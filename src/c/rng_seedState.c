#include "rng.h"
#include "variables.h"

/**
 * Fill the Mersenne state at g_RngMtState with a linear congruential sequence from seed
 * and clear the gameplay stream's remaining count.
 *
 * @romaddress 0x0803a608
 */
void rng_seedState(u32 seed)
{
    u32 *state;
    u32 i;

    seed |= 1;
    state = g_RngMtState;
    g_RngGameplayRemaining = 0;
    *state++ = seed;
    for (i = 623; i != 0; i--)
    {
        seed *= 69069;
        *state++ = seed;
    }
}
