#pragma once

#include "types.h"

void rng_allocState(void);
u32 rng_nextGameplay(void);
s32 rng_rollRange(s32 lo, s32 hi);
s32 rng_range(s32 lo, s32 hi);
bool rng_rollPercent(u16 percent);
s32 rng_mod(u16 n);
u32 rng_rollUpTo(u16 n);
void rng_reseed(void);

void rng_seedState(u32 seed);
u32 rng_nextMT19937(void);
s16 rng_signedOffset(u16 n);
u32 rng_refillGameplay(void);
