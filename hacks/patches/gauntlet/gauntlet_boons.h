#pragma once

#include "lib/combat_event.h"
#include "types.h"

/** Boon ids. Numbers in the design catalog are noted for the ones already built. */
enum GauntletBoonId
{
    GAUNTLET_BOON_EDGE_OF_ANDURIL, // 1
    GAUNTLET_BOON_DEEP_BREATH,     // 4
    GAUNTLET_BOON_HOBBIT_LUCK,     // 12
    GAUNTLET_BOON_CHAIN_LIGHTNING, // 13
    GAUNTLET_BOON_BLOODTHIRST,     // 14
    GAUNTLET_BOON_EXECUTIONER,     // 17
    GAUNTLET_BOON_CORPSE_BLAST,    // 20
    GAUNTLET_BOON_SECOND_WIND,     // 22
    GAUNTLET_BOON_BLOOD_PRICE,     // 36
    GAUNTLET_BOON_HASTE_LOOP,      // 39
    GAUNTLET_BOON_LAST_STAND,      // 60
    GAUNTLET_BOON_EYE_OF_SAURON,   // 62
    GAUNTLET_BOON_COUNT,
};

enum GauntletRarity
{
    RARITY_COMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY,
};

#define GAUNTLET_OFFER_COUNT 3
#define GAUNTLET_NO_BOON 0xff

/** Forget every boon, at the start of a run. */
void gauntletBoons_reset(void);

/** Recharge the once-per-room boons. */
void gauntletBoons_roomStart(void);

/** Fill offers with distinct unowned boons, at most one legendary. GAUNTLET_NO_BOON pads. */
void gauntletBoons_roll(u8 offers[GAUNTLET_OFFER_COUNT]);

void gauntletBoons_grant(u8 boon);

/** Grant a random unowned boon of one enum GauntletRarity, if any is left. */
void gauntletBoons_grantRandom(u8 rarity);
const char *gauntletBoons_name(u8 boon);
const char *gauntletBoons_description(u8 boon);
/** enum GauntletRarity */
u8 gauntletBoons_rarity(u8 boon);
const char *gauntletBoons_rarityName(u8 boon);
u32 gauntletBoons_ownedCount(void);

/** Timed boons, once per tick for the run's player. */
void gauntletBoons_tick(u8 playerIndex, Actor *owner, u32 room);

/** Player melee or ranged damage after the boon multipliers. */
s32 gauntlet_damageDealt(u8 playerIndex, Actor *victim, s32 damage);

/** Hit and kill triggers for a resolved player attack. */
void gauntlet_combatEvent(u8 playerIndex, const HackLibCombatEvent *event);

/** Boons that change the HP an enemy attack just removed from a player. */
void gauntlet_enemyHitResolved(u8 playerIndex, s32 hpBefore);

/** Current copies, 0 or 1 for unique boons. */
u32 gauntletBoons_stacks(u8 boon);
