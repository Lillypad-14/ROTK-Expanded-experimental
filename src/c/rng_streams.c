#include "rng.h"
#include "variables.h"

/**
 * Next tempered word of the gameplay stream, refilling the stream when exhausted. The low byte
 * of each word is also kept in g_RngGameplayLastByte.
 *
 * @romaddress 0x0803a640
 */
u32 rng_nextGameplay(void)
{
    u32 y;
    u32 out;
    u8 *last;

    if (--g_RngGameplayRemaining < 0)
    {
        return rng_refillGameplay();
    }
    y = *g_RngGameplayCursor++;
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    last = &g_RngGameplayLastByte;
    out = y ^ (y >> 18);
    *last = out;
    return out;
}

/**
 * Next tempered word of the MT19937 stream, rewinding to the start of the state when exhausted.
 *
 * @romaddress 0x0803a690
 */
u32 rng_nextMT19937(void)
{
    u32 y;

    if (--g_RngMtRemaining < 0)
    {
        g_RngMtRemaining = 623;
        g_RngMtCursor = g_RngMtState + 1;
    }
    y = *g_RngMtCursor++;
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= y >> 18;
    return y;
}
