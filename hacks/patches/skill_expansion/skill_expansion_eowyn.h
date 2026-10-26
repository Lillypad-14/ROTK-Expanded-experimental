#pragma once

// Eowyn page 2 "Against the Dark": the worse the odds around her, the stronger she fights. Odds
// are read live from the fight (nearby and bigger foes, Nazgul, missing HP) and set her tier.
// Rohirrim can also learn Gallop: running builds steps that her next melee hit spends.

#include "lib/combat_result.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

void skillExpansion_eowynReset(u8 playerIndex);
void skillExpansion_eowynCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_eowynTick(u8 playerIndex, Actor *actor);

/** Damage of a hit Eowyn deals to victim, with her tier, Gallop and Nazgul bonuses. */
s32 skillExpansion_eowynDamage(u8 playerIndex, Actor *victim, s32 damage, bool melee);

/** Damage a player takes. Stand Over the Fallen also shields Eowyn's partner. */
s32 skillExpansion_eowynIncomingDamage(u8 playerIndex, Actor *attacker, s32 damage);

/** Whether a hit on the player flinches them. */
bool skillExpansion_eowynFlinch(u8 playerIndex, bool flinchOnDamage);

/** A hit by the player's own attack or ability. */
void skillExpansion_eowynOnHit(u8 playerIndex, Actor *attacker, Actor *victim,
                               HackLibDamageResult result, bool melee);

/** Point Rohan Sprint and Forth Eorlingas at wrappers that feed Gallop and Odds. */
void skillExpansion_eowynWrapStockCallbacks(u8 playerIndex, ActiveSkill *rows);

/** The Odds tier and Gallop meters between Eowyn's HP and Spirit bars. */
void skillExpansion_eowynDrawHud(u8 playerIndex);

SkillExpansionCastResult skillExpansion_eowynDeathSeeker(Actor *playerActor);
SkillExpansionCastResult skillExpansion_eowynComeAndFaceMe(Actor *playerActor);
SkillExpansionCastResult skillExpansion_eowynLance(Actor *playerActor);
SkillExpansionCastResult skillExpansion_eowynStandOverTheFallen(Actor *playerActor);
SkillExpansionCastResult skillExpansion_eowynHopeRekindled(Actor *playerActor);
SkillExpansionCastResult skillExpansion_eowynNoMan(Actor *playerActor);

/** FALSE below 15% HP. */
bool32 skillExpansion_canCastEowynDeathSeeker(u8 playerIndex);

/** FALSE while Stand Over the Fallen is up. */
bool32 skillExpansion_canCastEowynStand(u8 playerIndex);

/** FALSE unless Eowyn is Desperate or Hopeless, and while I Am No Man! is up. */
bool32 skillExpansion_canCastEowynNoMan(u8 playerIndex);
