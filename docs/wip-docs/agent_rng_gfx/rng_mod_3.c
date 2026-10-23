#include "rng.h"

s32 rng_mod(u16 n)
{
    return (u32)((rng_nextGameplay() & 0x7fff) * (n + 1)) * 2 >> 16;
}
