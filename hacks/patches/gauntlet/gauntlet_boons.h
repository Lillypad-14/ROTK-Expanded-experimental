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
    GAUNTLET_BOON_HEARTY,          // 3
    GAUNTLET_BOON_IRON_SKIN,
    GAUNTLET_BOON_QUICK_STUDY, // 8
    GAUNTLET_BOON_SIPHON_MIND,
    GAUNTLET_BOON_VERMIN_SLAYER,
    GAUNTLET_BOON_THORNS,        // 16
    GAUNTLET_BOON_STATIC_CHARGE, // 21
    GAUNTLET_BOON_BATTLE_CRY,    // 33
    GAUNTLET_BOON_TROLL_SLAYER,  // 35
    GAUNTLET_BOON_RIPOSTE,       // 32
    GAUNTLET_BOON_MOMENTUM,      // 47
    GAUNTLET_BOON_MEDITATION,    // 48
    GAUNTLET_BOON_WARRIORS_REST,
    GAUNTLET_BOON_GLASS_CANNON, // 41
    GAUNTLET_BOON_BIG_GAME,     // 40
    GAUNTLET_BOON_BERSERKER,
    GAUNTLET_BOON_AVALANCHE,         // 59
    GAUNTLET_BOON_LIGHT_OF_EARENDIL, // 65
    GAUNTLET_BOON_KINGSFOIL_POUCH,   // 61
    GAUNTLET_BOON_SECOND_LIFE,
    GAUNTLET_BOON_SWIFT_BLADE,
    GAUNTLET_BOON_KEEN_EYE,
    GAUNTLET_BOON_FLEETFOOT,
    GAUNTLET_BOON_DWARF_GOLD,
    GAUNTLET_BOON_STEADY_FOOTING,
    GAUNTLET_BOON_LONG_REACH,
    GAUNTLET_BOON_RANGERS_PATIENCE,
    GAUNTLET_BOON_EMBER_TRAIL,
    GAUNTLET_BOON_ECHO_CAST,
    GAUNTLET_BOON_OVERKILL,
    GAUNTLET_BOON_KINDLING,
    GAUNTLET_BOON_BARROW_VENOM,
    GAUNTLET_BOON_CARADHRAS_CHILL,
    GAUNTLET_BOON_ORC_CLEAVER,
    GAUNTLET_BOON_BLACK_SPEECH,
    GAUNTLET_BOON_SHATTER,
    GAUNTLET_BOON_PLAGUE_SPREADER,
    GAUNTLET_BOON_WILDFIRE_SEED,
    GAUNTLET_BOON_GEM_MAGNET,
    GAUNTLET_BOON_WARG_HUNTER,
    GAUNTLET_BOON_SPLIT_SHOT,
    GAUNTLET_BOON_OVERFLOW,
    GAUNTLET_BOON_RITUAL,
    GAUNTLET_BOON_SUMMONERS_PACT,
    GAUNTLET_BOON_BOOMERANG,
    GAUNTLET_BOON_DETONATE,
    GAUNTLET_BOON_TWIN_SOULS,
    GAUNTLET_BOON_SIPHON,
    GAUNTLET_BOON_ELEMENTAL_WEAVE,
    GAUNTLET_BOON_GRAVITY_WELL,
    GAUNTLET_BOON_PHALANX,
    GAUNTLET_BOON_MIRROR_IMAGE,
    GAUNTLET_BOON_RAPID_FIRE,
    GAUNTLET_BOON_LAST_BREATH,
    GAUNTLET_BOON_WANDERING_STAR,
    GAUNTLET_BOON_THE_ONE_RING,
    GAUNTLET_BOON_WRAITH_FORM,
    GAUNTLET_BOON_BALROGS_WHIP,
    GAUNTLET_BOON_ARMY_OF_THE_DEAD,
    GAUNTLET_BOON_MITHRANDIRS_STAFF,
    GAUNTLET_BOON_STORM_PYRE,
    GAUNTLET_BOON_WILDFIRE,
    GAUNTLET_BOON_VOLLEY_ECHO,
    GAUNTLET_BOON_IRON_HIDE,
    GAUNTLET_BOON_PERMAFROST,
    GAUNTLET_BOON_PANDEMIC,
    GAUNTLET_BOON_REAPERS_DUE,
    GAUNTLET_BOON_SIEGE_ENGINE,
    GAUNTLET_BOON_ELVEN_VOLLEY,
    GAUNTLET_BOON_FAULT_LINE,
    GAUNTLET_BOON_YOU_SHALL_NOT_PASS,
    GAUNTLET_BOON_SPREADING_DREAD,
    GAUNTLET_BOON_VOICE_OF_COMMAND,
    GAUNTLET_BOON_HEIR_OF_ISILDUR,
    GAUNTLET_BOON_COUNT,
};

enum GauntletRarity
{
    RARITY_COMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY,
};

_Static_assert(GAUNTLET_BOON_COUNT <= 96, "the owned boon mask has three words");

#define GAUNTLET_OFFER_COUNT 3
#define GAUNTLET_NO_BOON 0xff

/** Forget every boon, at the start of a run. */
void gauntletBoons_reset(void);

/** Recharge the once-per-room boons. */
void gauntletBoons_roomStart(void);

/**
 * Fill offers with distinct unowned boons of at least minRarity, at most one legendary.
 * GAUNTLET_NO_BOON pads.
 */
void gauntletBoons_roll(u8 offers[GAUNTLET_OFFER_COUNT], u8 minRarity);

/** Roll at the preferred rarity, lowering the floor only when no eligible offer remains. */
u8 gauntletBoons_rollBestAvailable(u8 offers[GAUNTLET_OFFER_COUNT], u8 preferredRarity);

void gauntletBoons_grant(u8 boon);

/** Grant a random unowned boon of one enum GauntletRarity, if any is left. */
void gauntletBoons_grantRandom(u8 rarity);
const char *gauntletBoons_name(u8 boon);
const char *gauntletBoons_description(u8 boon);
/** enum GauntletRarity */
u8 gauntletBoons_rarity(u8 boon);
const char *gauntletBoons_rarityName(u8 boon);
u32 gauntletBoons_ownedCount(void);

/** Percent more XP the owned boons give. */
u32 gauntletBoons_xpBonusPercent(void);

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

/** TRUE when this boon belongs to the current run. */
bool32 gauntletBoons_has(u8 boon);

/** TRUE when an owned boon needs the effect workspace. */
bool32 gauntletBoons_hasExpanded(void);
