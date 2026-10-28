#pragma once

// Versus: player-vs-player for co-op.
//
// Friendly Fire decides when one player's hits land on the other through the stock
// combat_resolvePvpAttack (accuracy, parry, dodge, block, riposte, crits, knockdown), which the
// stock game only allows on Grueling. Both stock sites are hooked: the melee swing and the
// projectile/weapon-effect hit.
//
// Duel turns a co-op mission into a match: hits always land on the other player, a player brought
// to 0 HP is knocked out instead of dying, the other scores, and the knocked-out player respawns
// where they started. The first to the KO target wins and both go back to the co-op level select.

#include "actor.h"
#include "lib/combat_result.h"
#include "types.h"

enum VersusFriendlyFire
{
    VERSUS_FRIENDLY_FIRE_STOCK, // Grueling difficulty only, like the stock game
    VERSUS_FRIENDLY_FIRE_ALWAYS,
    VERSUS_FRIENDLY_FIRE_NEVER, // not even on Grueling (Smeagol still backstabs allies)
    VERSUS_FRIENDLY_FIRE_COUNT  // also the blank marker: the 0xFF fill reads 3 here
};

#define VERSUS_KO_TARGET_COUNT 4
#define VERSUS_DAMAGE_COUNT 4

/**
 * Persisted settings, one byte: the save area's versusState, sent in the co-op session options
 * (whose peer record has no byte to spare). A blank byte has friendlyFire 3 and reads as the
 * defaults.
 */
struct VersusState
{
    u8 friendlyFire : 2;
    u8 duel : 1;        // co-op missions are duels
    u8 duelKoTarget : 2; // index into the KOs-to-win table
    u8 duelEnemies : 1;  // keep the mission's enemies in a duel
    u8 duelDamage : 2;   // index into the damage table, for hits between players in a duel
};

extern struct VersusState g_VersusState;

/** Boot task (runTasks): load the persisted byte, a blank or invalid one becomes the defaults. */
void versus_loadState(void);

/**
 * The stock Grueling test at both player-vs-player sites, after the Smeagol checks: TRUE resolves
 * the hit as player vs player.
 */
bool32 versus_friendlyFireHits(void);

/** REPLACES (whole function): combat_resolvePvpAttack, marking the hit for the duel's damage. */
bool versus_resolvePvpAttack(Actor *attacker, Actor *victim);

/** scene_gameplay_enter, before the stock body: a fresh mission starts or ends a duel. */
void versus_gameplayEnter(void);

/** Per-frame duel work for one player, before the stock player tick. */
void versus_playerTick(u8 playerIndex);

/**
 * An HP change about to be applied to a player (player_addHp's delta): returns it adjusted for the
 * duel, 0 to drop it (a knocked-out or respawning player takes no damage).
 */
s32 versus_adjustHpChange(u8 playerIndex, s32 amount);

/** After an HP change was applied: a duel player at 0 HP is knocked out. */
void versus_afterHpChange(u8 playerIndex);

/**
 * The player whose code runs (its tick, which drives its casts, deliveries and timelines), or
 * VERSUS_NO_PLAYER. Returns the previous one, for versus_endActing.
 */
#define VERSUS_NO_PLAYER 0xff
u8 versus_beginActing(u8 playerIndex);
void versus_endActing(u8 previous);

/** skillExpansion_isFoe for a player avatar: the acting player's opponent in a running duel. */
bool32 versus_isDuelFoe(Actor *actor);

/** A skill's damage landing on a player avatar: a duel hit by the acting player, or nothing. */
HackLibDamageResult versus_skillHitsPlayer(Actor *victim, u16 damage);

/**
 * Co-op map select (the stock level select and the chapter pages), before their own tick: START
 * switches Duel on or off for this session and the host's setting. Returns TRUE when it took the
 * press. Both consoles see player 0's press, so both switch.
 */
bool32 versus_coopMenuTick(void);

/** The Duel line on the stock co-op level select's panel; drawn on entry and on each switch. */
void versus_drawCoopMenuLabel(void);

// The patch's mod_options page entries (compiled when HACK_mod_options).
#include "patches/mod_options/mod_options.h"

#define VERSUS_OPTION_COUNT 5
extern const struct ModOptionEntry VersusOptions[VERSUS_OPTION_COUNT];
