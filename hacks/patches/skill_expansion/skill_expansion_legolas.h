#pragma once

// Legolas page 2 "The Glint": loose the Overdraw on its glint for a True Shot. Page 2 actives nock
// an arrow that fires on the next loose.

#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

void skillExpansion_legolasReset(u8 playerIndex);
void skillExpansion_legolasCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_legolasTick(u8 playerIndex, Actor *actor);
void skillExpansion_legolasInput(u8 playerIndex);
s32 skillExpansion_legolasDamage(u8 playerIndex, Actor *victim, s32 damage, bool melee);
s32 skillExpansion_legolasIncomingDamage(u8 playerIndex, s32 damage);
bool skillExpansion_legolasFlinch(u8 playerIndex, bool flinchOnDamage);

/** Page 2 row of the active player's nocked arrow, or 0 when none is nocked. */
u8 skillExpansion_legolasNockedRow(void);

/** The cast refusal message of a Legolas row, or `message` when it has none of its own. */
const char *skillExpansion_legolasRejectMessage(u8 playerIndex, u8 row, const char *message);

/**
 * Advance Legolas's bow charge and start Overdraw when charged, with unique bows too.
 *
 * REPLACES (whole function): player_tickOverdrawCharge.
 */
s32 skillExpansion_legolasOverdrawCharge(s32 playerIndex, Actor *actor);

SkillExpansionCastResult skillExpansion_legolasNock(Actor *playerActor);
bool32 skillExpansion_canCastLegolasNock(u8 playerIndex);
