#pragma once

// Gimli's page 2, "The Tally": Stone, Fury and Tally builds. Its ranks are bought with kills
// instead of skill points. The Tally is the stock lifetime kill count, Player.killCount, so old
// saves keep theirs. Gimli also has a charge bar: every 10 kills empower his next active skill.
// Every entry point does nothing for other heroes.

#include "actor.h"
#include "lib/combat_result.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

// Kills per mark. A page 2 rank costs one mark.
#define GIMLI_TALLY_KILLS_PER_MARK 10

// Kills per required-level step. Page 2 requiredLevels of Gimli count in these steps.
#define GIMLI_TALLY_KILLS_PER_STEP 20

/** TRUE when the player's page 2 is bought with the Tally. */
bool32 skillExpansion_gimliUsesTally(u8 playerIndex);

/** Add bonus kills to the Tally, with the stock 500-kill reward. */
void skillExpansion_gimliAddTally(u8 playerIndex, u32 kills);

/** Raise the Tally to at least kills without per-kill effects or the 500-kill reward. */
void skillExpansion_gimliRaiseTally(u8 playerIndex, u16 kills);

void skillExpansion_gimliReset(u8 playerIndex);
void skillExpansion_gimliCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_gimliTick(u8 playerIndex, Actor *actor);

/** Damage of a hit Gimli deals to victim, with Grudges, Grudgebearer, Wager and empowerment. */
s32 skillExpansion_gimliDamage(u8 playerIndex, Actor *victim, s32 damage);

/**
 * Damage a player takes. Gimli's own guards act here, and Not Without Me takes a share of his
 * partner's hits.
 */
s32 skillExpansion_gimliIncomingDamage(u8 playerIndex, s32 damage);

/** Whether a hit on the player flinches them. Stubborn as Stone keeps Gimli steady. */
bool skillExpansion_gimliFlinch(u8 playerIndex, bool flinchOnDamage);

/** A kill by the attacking player's own hit. */
void skillExpansion_gimliOnKill(u8 playerIndex, Actor *victim);

SkillExpansionCastResult skillExpansion_gimliBarukKhazad(Actor *playerActor);
void skillExpansion_gimliHoldTheDeep(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gimliCleaveTheWeak(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gimliNobodyTossesADwarf(Actor *playerActor);
SkillExpansionCastResult skillExpansion_gimliBookOfGrudges(Actor *playerActor);
void skillExpansion_gimliStillCounting(Actor *playerActor);

/** FALSE while Hold the Deep is up. */
bool32 skillExpansion_canCastGimliHold(u8 playerIndex);

/** FALSE while Still Counting is up. */
bool32 skillExpansion_canCastGimliCounting(u8 playerIndex);

/** An active skill was paid for. A full charge bar empowers it. */
void skillExpansion_gimliCastStarted(u8 playerIndex);

/** The cast ended. An empowered success spends the charge and refunds the Spirit. */
void skillExpansion_gimliCastEnded(u8 playerIndex, bool success);

/** The charge bar between Gimli's HP and Spirit bars. It glints when full. */
void skillExpansion_gimliDrawTallyBar(u8 playerIndex);
