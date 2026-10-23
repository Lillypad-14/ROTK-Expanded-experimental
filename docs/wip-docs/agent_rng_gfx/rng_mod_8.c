#include "rng.h"

s32 rng_mod(u16 n)
{
    u32 roll = rng_nextGameplay() & 0x7fff; roll *= n + 1; return (roll * 2) >> 16;
}
