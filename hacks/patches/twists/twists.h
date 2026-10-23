#pragma once

// Twists: two mod_options pages of persistent playthrough modifiers (damage scaling, life
// drain, berserker and momentum bonuses, regeneration, kill rewards), applied to every player.

#include "lib/combat_event.h"
#include "types.h"

/** Boot task (runTasks): load the persisted settings, an untagged area becomes the defaults. */
void twists_loadState(void);

/** Gameplay start for one player: clear the run state and roll the level's omen. */
void twists_missionBegin(u8 playerIndex);

/** Per-frame timers and regeneration for one player. */
void twists_playerTick(u8 playerIndex);

/** Player melee or ranged damage after the damage-dealt twists. */
s32 twists_damageDealt(u8 playerIndex, s32 damage);

/** Rescale the HP an enemy attack just removed from a player. */
void twists_enemyHitResolved(u8 playerIndex, s32 hpBefore);

/** player_awardXp with the XP Gain twist applied. */
void twists_awardXp(s16 xp);

/** Life drain, kill rewards and the momentum streak for a resolved player attack. */
void twists_combatEvent(u8 playerIndex, const HackLibCombatEvent *event);

// The patch's mod_options page entries (compiled when HACK_mod_options).
#include "patches/mod_options/mod_options.h"

#define TWISTS_COMBAT_OPTION_COUNT 6
#define TWISTS_VIGOR_OPTION_COUNT 6
#define TWISTS_FATE_OPTION_COUNT 4
extern const struct ModOptionEntry TwistsCombatOptions[TWISTS_COMBAT_OPTION_COUNT];
extern const struct ModOptionEntry TwistsVigorOptions[TWISTS_VIGOR_OPTION_COUNT];
extern const struct ModOptionEntry TwistsFateOptions[TWISTS_FATE_OPTION_COUNT];
