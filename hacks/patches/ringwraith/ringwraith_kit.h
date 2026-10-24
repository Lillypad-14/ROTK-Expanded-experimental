#pragma once

// Ringwraith skill kit. Foes carry Dread that the wraith's presence, blows and skills build up.
// Frightened foes hit weaker and terrified ones freeze. The wraith heals only by draining, and
// Spirit comes only from kills.

#include "actor.h"
#include "patches/ringwraith/ringwraith.h"
#include "patches/skill_expansion/skill_expansion.h"

extern const SkillExpansionHeroKit RingwraithHeroKit;

// Enemy whose melee attack on a player is resolving, or NULL.
extern Actor *ringwraith_kitEnemyAttacker;

void ringwraith_kitReset(u8 playerIndex);

/** While on, the wraith accepts the game's full heal for a fresh scene start. */
void ringwraith_kitSetFullHeal(bool32 on);
void ringwraith_kitTick(u8 playerIndex, Actor *playerActor);
void ringwraith_tickThrall(Actor *thrall);

/** Melee damage after the wraith's Dread and stealth bonuses. */
s32 ringwraith_kitMeleeDamage(u8 playerIndex, Actor *victim, s32 damage);

/** Projectile and thrall damage after the wraith's Dread Lord bonus. */
s32 ringwraith_kitRangedDamage(u8 playerIndex, Actor *victim, s32 damage);

// Page 2 actives, in RingwraithRow order.
extern const SkillExpansionActiveDef RingwraithCommandOfTheNine;
extern const SkillExpansionActiveDef RingwraithShadowstep;
extern const SkillExpansionActiveDef RingwraithShadowOfMordor;
extern const SkillExpansionActiveDef RingwraithBlackSacrifice;
extern const SkillExpansionActiveDef RingwraithWillOfSauron;

void ringwraith_kitOnHit(u8 playerIndex, Actor *attacker, Actor *victim, bool32 killed,
                         bool32 melee, s32 damageDealt);
