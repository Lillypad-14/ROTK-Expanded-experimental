#include "rng.h"
#include "variables.h"

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
