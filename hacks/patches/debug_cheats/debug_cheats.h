#pragma once

// Debug Cheats: a mod_options page of persistent testing toggles (max level, infinite points,
// god mode, infinite spirit, one-hit kills, max kills), applied every player tick.

#include "actor.h"
#include "types.h"

/**
 * State byte layout: one bit per toggle, bit 7 the tag: 0 once written, 1 on a blank byte
 * (the 0xFF migration fill), so an unwritten byte reads as all toggles off.
 */
struct DebugCheatsState
{
    u8 maxLevel : 1;
    u8 skillPoints : 1;
    u8 attributePoints : 1;
    u8 godMode : 1;
    u8 infiniteSpirit : 1;
    u8 oneHitKills : 1;
    u8 maxKills : 1;
    u8 tag : 1;
};

#define DEBUG_CHEATS_TAG 0 // written bytes read tag 0; the 0xFF blank fill reads tag 1

extern struct DebugCheatsState g_DebugCheatsState;

/** Boot task (runTasks): load the persisted state byte, an untagged byte turns all off. */
void debugCheats_loadState(void);

/** Apply the level, point, HP, Spirit and kill cheats to one player. */
void debugCheats_playerTick(u8 playerIndex);

/** Melee damage after One-Hit Kills. */
s32 debugCheats_meleeDamage(Actor *victim, s32 damage);

// The patch's mod_options page entries (compiled when HACK_mod_options).
#include "patches/mod_options/mod_options.h"

#define DEBUG_CHEATS_OPTION_COUNT 7
extern const struct ModOptionEntry DebugCheatsOptions[DEBUG_CHEATS_OPTION_COUNT];
