#include "rng.h"

s32 rng_mod(u16 n)
{
    u32 roll = rng_nextGameplay(); return ((roll & 0x7fff) * (n + 1) * 2) >> 16;
}
