#include "rng.h"

s32 rng_mod(u16 n)
{
    u32 roll = 0x7fff & rng_nextGameplay(); return (roll * (n + 1) * 2) >> 16;
}
