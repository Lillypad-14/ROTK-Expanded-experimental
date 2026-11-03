#pragma once

// Skill kits written as data (PC edition). hacks/tools/hero_kit.py compiles
// hacks/content/heroes/<hero>.json into kit tables run by this engine. See
// docs/wip-docs/howto-heroes-skills.md, "Skill specs".

#include "patches/skill_expansion/skill_delivery.h"
#include "patches/skill_expansion/skill_expansion.h"

typedef enum SpecEffect
{
    // Launch a delivery: a projectile, or a burst at the caster or the nearest foe.
    SPEC_EFFECT_DELIVERY = 0,
    // Heal the caster by amount HP.
    SPEC_EFFECT_HEAL = 1,
    // The caster takes amount percent less damage for frames.
    SPEC_EFFECT_WARD = 2,
    // The caster's melee hits deal amount percent more for frames.
    SPEC_EFFECT_EMPOWER = 3,
} SpecEffect;

typedef struct SpecSkill
{
    // SPEC_EFFECT_DELIVERY only.
    const SkillDeliveryDef *delivery;
    // Per skill level 1..5: HP healed, or the ward or empower percent.
    u16 amount[5];
    // Ward or empower duration.
    u16 frames;
    // SpecEffect.
    u8 effect;
    // Nonzero: a delivery launched at the nearest foe within this many pixels instead of the caster.
    u8 targetRange;
    // Cast flourish on the caster: an fx_kit shape and color.
    u8 fxShape;
    u8 fxColor;
} SpecSkill;

#define SPEC_KIT_CAST_ROW(cost)                                                                    \
    {.spiritCost = cost,                                                                           \
     .field_0x20 = 1,                                                                              \
     .castFlags = {.d = {.forcePose0 = 1}},                                                        \
     .field_0x22 = 5,                                                                              \
     .castTriggerFrame = 0,                                                                        \
     .field_0x24 = 0xff}

extern const SkillExpansionCastPolicy SpecKitQuickPolicy;
extern const SkillExpansionCastPolicy SpecKitNormalPolicy;
extern const SkillExpansionCastPolicy SpecKitLongPolicy;
extern const SkillExpansionCastPolicy SpecKitUltimatePolicy;

SkillExpansionCastResult specKit_resolve(Actor *actor);
void specKit_reset(u8 playerIndex);
void specKit_cleanup(u8 playerIndex, Actor *actor);
void specKit_tick(u8 playerIndex, Actor *actor);
s32 specKit_meleeDamage(u8 playerIndex, Actor *victim, s32 damage);
s32 specKit_incomingDamage(u8 playerIndex, Actor *attacker, s32 damage);
