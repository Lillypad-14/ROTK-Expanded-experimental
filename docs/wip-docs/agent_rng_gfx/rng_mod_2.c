#include "rng.h"

s32 rng_mod(u16 n)
{
    u32 m = n + 1; return ((rng_nextGameplay() & 0x7fff) * m * 2) >> 16;
}
