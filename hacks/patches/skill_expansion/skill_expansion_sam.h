#pragma once

// Sam's page 2: Gardener, Stout Hobbit and Cook builds around Samwise's Garden. Sam plants taters
// and brambles that grow sprout, bloom, then ripe, faster while he stands near. Every entry point
// does nothing for other heroes.

#include "actor.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "types.h"

void skillExpansion_samReset(u8 playerIndex);
void skillExpansion_samCleanup(u8 playerIndex, Actor *actor);
void skillExpansion_samTick(u8 playerIndex, Actor *actor);

/** Damage of a hit Sam deals, with the stew buff and Shire Grit. */
s32 skillExpansion_samDamage(u8 playerIndex, s32 damage, bool melee);

/** Damage Sam takes. Parries, Stubborn and I Can Carry You act here. */
s32 skillExpansion_samIncomingDamage(u8 playerIndex, Actor *origin, s32 damage);

void skillExpansion_samPlantTaters(Actor *playerActor);
void skillExpansion_samBrambleHedge(Actor *playerActor);
void skillExpansion_samSecondBreakfast(Actor *playerActor);
void skillExpansion_samFryingPanParry(Actor *playerActor);
SkillExpansionCastResult skillExpansion_samElvenRope(Actor *playerActor);
SkillExpansionCastResult skillExpansion_samPoTayToes(Actor *playerActor);

/** FALSE while Frying Pan Parry is up. */
bool32 skillExpansion_canCastSamParry(u8 playerIndex);
