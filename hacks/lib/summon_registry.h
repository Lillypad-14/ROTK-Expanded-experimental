#pragma once

#include "lib/targeting.h"

#define HACKLIB_SUMMON_LIMIT 8

typedef struct HackLibSummonDef
{
    Actor *(*spawn)(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime, u16 damage);
    void (*tick)(Actor *actor, Actor *target, u16 damage);
    void (*despawn)(Actor *actor);
    bool32 (*isUsable)(Actor *actor);
    HackLibTargetFilter filter;
    u16 targetRadius;
    // Native lifetime/count bookkeeping stays with the adapter when FALSE.
    bool managedLifetime;
} HackLibSummonDef;

typedef struct HackLibSummon
{
    const HackLibSummonDef *definition;
    Actor *actor;
    Actor *owner;
    ActorCallback originalTick;
    u16 remaining;
    u16 damage;
    u8 abilityRow;
    u32 ownerGeneration;
    u32 actorGeneration;
} HackLibSummon;

// The cap applies to this definition's usable summons owned by this player.
Actor *hackLib_spawnSummon(const HackLibSummonDef *definition, Actor *owner, Vector2Fp16 *position,
                           Actor *target, u16 lifetime, u16 damage, u8 cap, u8 abilityRow);
u32 hackLib_countSummons(Actor *owner, const HackLibSummonDef *definition);
void hackLib_clearOwnerSummons(Actor *owner);
u32 hackLib_summonPoolUsed(void);
