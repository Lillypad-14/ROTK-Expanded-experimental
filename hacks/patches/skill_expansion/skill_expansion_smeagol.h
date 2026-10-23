#pragma once

// Smeagol's page 2: Smeagol, Gollum and Precious builds around Gollum's Grip (0..100). The grip
// climbs in combat, tints Smeagol toward Gollum, and at 100 Gollum takes over for a few seconds.
// Every entry point does nothing for other heroes.

#include "actor.h"
#include "lib/combat_result.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

void skillExpansion_smeagolReset(u8 playerIndex);
void skillExpansion_smeagolCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_smeagolTick(u8 playerIndex, Actor *actor);

/** Rewrite the keys of a Smeagol whose body Gollum, Throttle or Play Dead controls. */
void skillExpansion_smeagolInput(u8 playerIndex);

/** Damage of a hit Smeagol deals, with the Hungry, takeover and Play Dead bonuses. */
s32 skillExpansion_smeagolDamage(u8 playerIndex, s32 damage, bool melee);
void skillExpansion_smeagolOnHit(u8 playerIndex, Actor *victim, HackLibDamageResult result,
                                 bool melee);

/** Damage Smeagol takes. Hits feed the grip and Smeagol Is Free blocks them. */
s32 skillExpansion_smeagolIncomingDamage(u8 playerIndex, s32 damage);

void skillExpansion_smeagolPlayDead(Actor *playerActor);
void skillExpansion_smeagolCatchAFish(Actor *playerActor);
SkillExpansionCastResult skillExpansion_smeagolThrottle(Actor *playerActor);
SkillExpansionCastResult skillExpansion_smeagolSlink(Actor *playerActor);
void skillExpansion_smeagolDropThePrecious(Actor *playerActor);
void skillExpansion_smeagolArgueWithYourself(Actor *playerActor);

/** FALSE while Gollum holds the body or a skill's effect is still running. */
bool32 skillExpansion_canCastSmeagolSkill(u8 playerIndex);
