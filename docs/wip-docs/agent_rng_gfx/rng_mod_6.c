#include "rng.h"

s32 rng_mod(u16 n)
{
    return ((0x7fff & rng_nextGameplay()) * (n + 1) * 2) >> 16;
}
