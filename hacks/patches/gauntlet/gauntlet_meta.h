#pragma once

// Gauntlet progress kept between runs: Embers, permanent unlocks and per-hero bests. It lives in
// the global mod options save area and is read from the chip when needed, so it costs no RAM.

#include "types.h"

#define GAUNTLET_META_HERO_COUNT 10

enum GauntletUnlock
{
    // +3 Health per rank at the start of a run
    GAUNTLET_UNLOCK_VITALITY,
    // +10% of max HP healed per rank when a room is cleared
    GAUNTLET_UNLOCK_MENDING,
    // start with a random boon: Common at rank 1, Rare at rank 2
    GAUNTLET_UNLOCK_BLESSING,
    // one boon reroll per rank in each run
    GAUNTLET_UNLOCK_FATE,
    // start with a second skill
    GAUNTLET_UNLOCK_VETERAN,
    GAUNTLET_UNLOCK_COUNT,
};

typedef struct GauntletMeta
{
    // GAUNTLET_META_TAG once written. A blank area reads 0xFF.
    u8 tag;
    // HeroId picked last in the hub
    u8 lastHero;
    u8 embersLow;
    u8 embersHigh;
    // most rooms cleared in one run, per HeroId
    u8 best[GAUNTLET_META_HERO_COUNT];
    // rank per enum GauntletUnlock
    u8 unlocks[GAUNTLET_UNLOCK_COUNT];
    u8 runsLow;
    u8 runsHigh;
} GauntletMeta;

void gauntletMeta_load(GauntletMeta *meta);
void gauntletMeta_save(const GauntletMeta *meta);

u32 gauntletMeta_embers(const GauntletMeta *meta);
u32 gauntletMeta_runs(const GauntletMeta *meta);

/** Rank of one unlock, read from the save. */
u32 gauntletMeta_rank(u32 unlock);

const char *gauntletMeta_unlockName(u32 unlock);
const char *gauntletMeta_unlockDescription(u32 unlock);
u32 gauntletMeta_unlockMaxRank(u32 unlock);

/** Embers the next rank costs, or 0 at max rank. */
u32 gauntletMeta_unlockCost(const GauntletMeta *meta, u32 unlock);

/** Buy the next rank and save. FALSE when it is maxed or too expensive. */
bool32 gauntletMeta_buy(GauntletMeta *meta, u32 unlock);

/** Embers a finished run earns. */
u32 gauntletMeta_runReward(u32 roomsCleared, u32 kills);

/** Bank a finished run: Embers, run count and the hero's best. TRUE for a new best. */
bool32 gauntletMeta_recordRun(u32 heroId, u32 roomsCleared, u32 kills);
