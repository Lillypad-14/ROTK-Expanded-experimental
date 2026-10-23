#pragma once

#include "actor.h"

#define HACKLIB_EFFECT_LIMIT 12

enum HackLibEffectStackRule
{
    HACKLIB_EFFECT_REFRESH = 0,
    HACKLIB_EFFECT_STACK = 1,
    HACKLIB_EFFECT_REPLACE = 2,
};

typedef struct HackLibTimedEffect HackLibTimedEffect;
typedef struct HackLibEffectDef
{
    u16 id;
    u16 interval;
    u8 maxStacks;
    u8 stackRule;
    void (*apply)(HackLibTimedEffect *effect);
    void (*pulse)(HackLibTimedEffect *effect);
    void (*remove)(HackLibTimedEffect *effect);
    // Zero preserves ordinary movement. Otherwise 1..100 percent of tick displacement.
    u8 movementPercent;
} HackLibEffectDef;

struct HackLibTimedEffect
{
    const HackLibEffectDef *definition;
    Actor *owner;
    Actor *target;
    ActorCallback originalTick;
    u16 remaining;
    u16 untilPulse;
    s16 magnitude;
    u8 stacks;
    u8 abilityRow;
    u32 ownerGeneration;
    u32 targetGeneration;
};

// Definitions must live in ROM. Callbacks may deal damage but must not mutate the effect pool.
HackLibTimedEffect *hackLib_applyEffect(const HackLibEffectDef *definition, Actor *owner,
                                        Actor *target, u16 frames, s16 magnitude, u8 abilityRow);
void hackLib_removeEffect(HackLibTimedEffect *effect);
void hackLib_clearOwnerEffects(Actor *owner);
u32 hackLib_countEffects(Actor *owner);
u32 hackLib_effectPoolUsed(void);
const HackLibTimedEffect *hackLib_effectAt(u32 index);
bool hackLib_actorHasTick(Actor *actor, ActorCallback callback);
void hackLib_replaceActorTick(Actor *actor, ActorCallback before, ActorCallback after);
