#pragma once

#include "patches/skill_expansion/skill_expansion_elite.h"

typedef bool32 (*SkillExpansionTargetCondition)(Actor *target, u32 argument);

typedef struct SkillExpansionBurst
{
    Actor *attacker;
    Actor *center;
    Actor *exclude;
    u16 radius;
    u16 damage;
    u16 stunFrames;
    u8 kind;
    u8 abilityRow;
} SkillExpansionBurst;

Actor *skillExpansion_findTarget(Actor *origin, u16 range, SkillExpansionTargetCondition condition,
                                 u32 argument);
u32 skillExpansion_burstDamage(const SkillExpansionBurst *burst);
Actor *skillExpansion_launchProjectile(Actor *owner, u8 direction32, u16 damage, u8 abilityRow);
Actor *skillExpansion_summonGhost(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime,
                                  u16 damage, u8 cap, bool persistent);
bool skillExpansion_consumeGhost(Actor *ghost, Actor *owner);
u32 skillExpansion_projectilePoolUsed(void);
