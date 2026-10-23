#include "rng.h"

u32 rng_rollUpTo(u16 n)
{
    u32 roll = rng_nextMT19937() & 0x7fff;

    roll *= n + 1;
    return (roll * 2) >> 16;
}
