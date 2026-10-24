#pragma once

// Self-driving Gauntlet runs for hacks/tools/gauntlet_run.py. The runner patches
// gauntletTest_config in a copy of the ROM. The hero cannot die, foes fall on a timer, picks are
// taken automatically, and each room prints a line through mGBA's debug log.

#include "actor.h"
#include "types.h"

typedef struct GauntletTestConfig
{
    // Rooms to clear before the hero is felled, or 0 for normal play.
    u32 rooms;
    // HeroId of the run.
    u32 hero;
    // 1 to play every room map in table order instead of at random.
    u32 probeMaps;
    // 1 to start with every unlock at max rank.
    u32 allUnlocks;
} GauntletTestConfig;

extern const volatile GauntletTestConfig gauntletTest_config;

/** TRUE when the ROM boots straight into a test run. */
bool32 gauntletTest_isActive(void);

/** Boot into a test run instead of the title screens. */
void gauntletTest_boot(void);

bool32 gauntletTest_probesMaps(void);

/** Test play for one gameplay tick of the run. */
void gauntletTest_tick(Actor *owner);

/** Keys the test presses on the Gauntlet scene this tick, for an enum GauntletPage. */
u16 gauntletTest_scenePress(u32 page, u32 cursor);

void gauntletTest_roomCleared(u32 room, u32 mission, u32 spawned, u32 landFails, u32 purged);
void gauntletTest_runOver(u32 room, u32 kills);
void gauntletTest_finished(void);
