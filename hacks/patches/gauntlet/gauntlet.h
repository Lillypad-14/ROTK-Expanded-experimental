#pragma once

// Gauntlet: a roguelike mode started from the main menu (the GCN Link row). A run is a fresh hero
// fighting waves of foes room after room, each room on its own map, until it falls.

#include "actor.h"
#include "player.h"
#include "rng.h"
#include "types.h"

/** Gauntlet.mode values. */
enum GauntletMode
{
    GAUNTLET_MODE_OFF = 0,
    GAUNTLET_MODE_RUNNING = 1,
};

/** A gameplay-stream roll in [0, n). rng_mod's own range includes n. */
static inline u32 gauntlet_randBelow(u16 n)
{
    return rng_mod(n - 1);
}

/** Pick screen pages (the mod scene's arg 0). */
enum GauntletPage
{
    GAUNTLET_PAGE_BOON,
    GAUNTLET_PAGE_LEVEL,
    GAUNTLET_PAGE_RESULTS,
    GAUNTLET_PAGE_HUB,
    GAUNTLET_PAGE_UNLOCKS,
    GAUNTLET_PAGE_BRIEFING = 5,
};

/** Per-player tick hook (skillExpansion_playerTick) driving waves and rooms during a run. */
void gauntlet_playerTick(u8 playerIndex);

/** TRUE while a run is in progress. */
bool32 gauntlet_isRunning(void);

/** XP award scale in percent: faster leveling during a run. */
s32 gauntlet_xpPercent(void);

/** Start a run with a fresh hero on the first room's map. */
void gauntlet_startRun(HeroId heroId);

/** TRUE for a living foe this run spawned. */
bool32 gauntlet_isLivingRunFoe(Actor *actor);

/** Count a foe slain by the player during a run. */
void gauntlet_noteKill(void);

// Run state for the pick screen (gauntlet_scene.c).

/** Rooms cleared so far. */
u32 gauntlet_roomsCleared(void);
u32 gauntlet_kills(void);

/** TRUE and one queued level-up taken when a level-up reward is still to be picked. */
bool32 gauntlet_takeLevelPick(void);

/** Leave the pick screen for the next room's map. */
void gauntlet_enterNextRoom(void);

/** Select the next map once so the briefing and loaded room agree. */
void gauntlet_prepareNextRoom(void);
const char *gauntlet_roomName(void);
const char *gauntlet_encounterName(void);
/** Next floor waypoint while exploring, also used by the automated traversal check. */
bool32 gauntlet_travelTarget(Vector2Fp16 *position);

/** HeroId of the current or last run. */
u32 gauntlet_hero(void);

u32 gauntlet_rerollsLeft(void);

/** TRUE and one reroll spent when the run has one left. */
bool32 gauntlet_useReroll(void);

/** End the run state after the results screen. */
void gauntlet_finishRun(void);

/** Native compass frame, or 8 to preserve campaign tracking. */
u32 gauntlet_compassFrame(u8 playerIndex);
