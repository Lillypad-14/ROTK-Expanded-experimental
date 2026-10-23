#pragma once

#include "lib/actor_ref.h"

#define SKILL_DELIVERY_LIMIT 4
#define SKILL_DELIVERY_TARGET_LIMIT 8
#define SKILL_DELIVERY_NO_EFFECT 0xffff

enum SkillDeliveryKind
{
    SKILL_DELIVERY_STRAIGHT = 0,
    SKILL_DELIVERY_BURST = 1,
};

typedef struct SkillDeliveryDef
{
    u16 damage[5];
    u16 lifetime;
    u16 travelEffect;
    u16 trailEffect;
    u16 impactEffect;
    u16 launchSound;
    u16 impactSound;
    u16 stunTicks;
    u8 kind;
    u8 speed;
    u8 radius;
    u8 splashRadius;
    u8 maxTargets;
    u8 pierceTargets;
    u8 maxRepeats;
    u8 repeatTicks;
    u8 trailTicks;
    u8 knockback;
    bool stopOnWalls;
    u8 elements;
    bool elementsKnown;
    u8 status;
    u16 statusTicks;
    u16 statusPower[5];
    u16 cooldownTicks;
} SkillDeliveryDef;

typedef struct SkillDeliveryStats
{
    u32 contacts;
    u32 hits;
    u32 damage;
    u8 active;
} SkillDeliveryStats;

// Definitions are copied at launch. Level is 1..5. Direction uses the 32-way facing grid.
bool skillDelivery_launch(u8 playerIndex, const SkillDeliveryDef *definition,
                          const Vector2Fp16 *position, u8 direction, u8 level, u8 abilityRow,
                          bool cosmetic);
void skillDelivery_tick(u8 playerIndex, Actor *owner);
void skillDelivery_clear(u8 playerIndex);
void skillDelivery_getStats(u8 playerIndex, SkillDeliveryStats *out);
void skillDelivery_resetStats(u8 playerIndex);
extern const SkillDeliveryDef SkillDeliveryDefinitions[];
extern const char *const SkillDeliveryNames[];
#include "skill_delivery_ids.h"
