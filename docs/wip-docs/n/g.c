#include "rng.h"
#include "variables.h"

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
