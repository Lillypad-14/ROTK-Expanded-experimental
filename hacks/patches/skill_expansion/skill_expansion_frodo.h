#pragma once

// Frodo's page 2: Good, Evil and Misdirection builds around Burden, the stock corruption meter
// (Player.corruption, 0..100). Evil fills it, Good burns it away. Every entry point does nothing
// for other heroes.

#include "actor.h"
#include "lib/combat_result.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

// Burden at which the stock game buffs every enemy (GAME_FLAG_CORRUPTION_TIER_1).
#define FRODO_BURDEN_HEAVY 51

// Burden at which the stock game sends a Nazgul (GAME_FLAG_CORRUPTION_TIER_3).
#define FRODO_BURDEN_CONSUMED 91


void skillExpansion_frodoReset(u8 playerIndex);
void skillExpansion_frodoCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_frodoTick(u8 playerIndex, Actor *actor);
void skillExpansion_frodoWrapStockCallbacks(u8 playerIndex, ActiveSkill *rows);

/** Damage of a hit Frodo deals, with the Burden, Sting, shadow form and Unwavering bonuses. */
s32 skillExpansion_frodoDamage(u8 playerIndex, s32 damage, bool melee);
void skillExpansion_frodoOnHit(u8 playerIndex, Actor *victim, HackLibDamageResult result,
                               bool melee);

/** Damage Frodo takes after Claim It's rescue from a killing blow. */
s32 skillExpansion_frodoIncomingDamage(u8 playerIndex, Actor *attacker, s32 damage);

void skillExpansion_frodoPhialOfGaladriel(Actor *playerActor);
void skillExpansion_frodoElbereth(Actor *playerActor);
void skillExpansion_frodoShadowOfTheRing(Actor *playerActor);
SkillExpansionCastResult skillExpansion_frodoSowDiscord(Actor *playerActor);
SkillExpansionCastResult skillExpansion_frodoSmokeAndMirrors(Actor *playerActor);
SkillExpansionCastResult skillExpansion_frodoPutOnTheRing(Actor *playerActor);

bool32 skillExpansion_canCastPhialOfGaladriel(u8 playerIndex);

/** Draw the Burden bar between the HP and Spirit bars. Call once per HUD tick. */
void skillExpansion_frodoDrawBurdenBar(u8 playerIndex);
