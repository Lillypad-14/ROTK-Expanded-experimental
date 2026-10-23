#include "rng.h"
#include "variables.h"

u32 rng_refillGameplay(void)
{
    u32 *p = g_RngMtState;
    u32 *next = p + 2;
    u32 *mid = p + 397;
    u32 prev;
    u32 cur;
    u32 y;
    s32 i;

    if (g_RngGameplayRemaining < -1)
    {
        u32 seed = g_RngSeed | 1;
        u32 *q;

        g_RngGameplayRemaining = 0;
        q = p;
        *q++ = seed;
        for (i = 623; i != 0; i--)
        {
            seed *= 69069;
            *q++ = seed;
        }
    }
    g_RngGameplayRemaining = 623;
    g_RngGameplayCursor = g_RngMtState + 1;
    prev = g_RngMtState[0];
    cur = g_RngMtState[1];
    for (i = 227; i != 0; i--)
    {
        y = ((prev & 0x80000000) | (cur & 0x7fffffff)) >> 1;
        y ^= *mid++;
        *p++ = (cur & 1) ? y ^ 0x9908b0df : y;
        prev = cur;
        cur = *next++;
    }
    mid = g_RngMtState;
    for (i = 396; i != 0; i--)
    {
        y = ((prev & 0x80000000) | (cur & 0x7fffffff)) >> 1;
        y ^= *mid++;
        *p++ = (cur & 1) ? y ^ 0x9908b0df : y;
        prev = cur;
        cur = *next++;
    }
    cur = g_RngMtState[0];
    y = ((prev & 0x80000000) | (cur & 0x7fffffff)) >> 1;
    y ^= *mid;
    *p = y;
    if (cur & 1)
    {
        y ^= 0x9908b0df;
        *p = y;
    }
    cur ^= cur >> 11;
    cur ^= (cur << 7) & 0x9d2c5680;
    cur ^= (cur << 15) & 0xefc60000;
    cur ^= cur >> 18;
    return cur;
}
