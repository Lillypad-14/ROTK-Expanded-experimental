#pragma once

#include "lib/cast_timeline.h"
#include "patches/mod_options/mod_options.h"

void abilityBench_playerTick(u8 playerIndex);
void abilityBench_endSession(void);
const HackLibCastTimeline *abilityBench_getTimeline(u8 playerIndex, u8 row);
#define ABILITY_BENCH_OPTION_COUNT 7
extern const struct ModOptionEntry AbilityBenchOptions[ABILITY_BENCH_OPTION_COUNT];
