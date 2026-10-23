#include "lib/actor_ref.h"
#include "lib/combat_result.h"
#include "lib/combat_source.h"
#include "lib/profile.h"
#include "lib/summon_registry.h"
#include "lib/timed_effect.h"
#include "patches/skill_expansion/skill_expansion_save_format.h"
#include "patches/skill_expansion/skill_status.h"
#include "player.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ActorList g_ActorListMain;
PlayerGlobals g_PlayerGlobals;
static Actor actors[24];
ActorPoolMain *g_ActorPoolMain = (ActorPoolMain *)actors;
ActorPoolOverlay *g_ActorPoolOverlay;
static u32 profileClock;
u32 hackLib_profileClock(void)
{
    return profileClock;
}
static u32 pulses, removals, spawns, despawns, nativeTicks;
static bool failAllocation;

void *memory_memset(void *destination, u32 value, size_t size)
{
    return memset(destination, value, size);
}

void *memory_zalloc(size_t size)
{
    return failAllocation ? NULL : calloc(1, size);
}

void memory_free(void *pointer)
{
    free(pointer);
}

Actor *actor_iterateInRadius(Actor *center, Actor *cursor, s32 radius)
{
    (void)center;
    (void)cursor;
    (void)radius;
    // Shared targeting must work independently of the native radius iterator.
    return NULL;
}

static void nativeTick(Actor *actor)
{
    actor->coords.x += 4 * FP16_16_ONE;
    nativeTicks++;
}

static void resetActors(void)
{
    memset(actors, 0, sizeof(actors));
    for (u32 i = 0; i < ARRAY_COUNT(actors); i++)
    {
        actors[i].next = i + 1 < ARRAY_COUNT(actors) ? &actors[i + 1] : NULL;
        actors[i].currentHp = 100;
        actors[i].tickCallback = nativeTick;
        hackLib_noteActorAllocated(&actors[i]);
    }
    g_ActorListMain.head = actors;
    PLAYER(0).ownerActor = actors;
    PLAYER(0).currentHp = 100;
    actors[0].currentHp = 0;
}

static bool32 enemyOnly(Actor *target, u32 argument)
{
    return target != &actors[argument];
}

static void checkTargeting(void)
{
    HackLibTargetQuery query = {
        .center = &actors[0], .radius = 10, .filter = enemyOnly, .argument = 3};
    for (u32 i = 1; i < ARRAY_COUNT(actors); i++)
    {
        actors[i].coords.x = 100 * FP16_16_ONE;
    }
    actors[1].coords.x = 6 * FP16_16_ONE;
    actors[1].coords.y = 8 * FP16_16_ONE;
    actors[2].coords.x = -5 * FP16_16_ONE;
    actors[3].coords.x = FP16_16_ONE;
    assert(hackLib_nearestTarget(&query) == &actors[2]);
    query.facing.x = 1;
    query.coneCosine = 181;
    assert(hackLib_nearestTarget(&query) == NULL);
    query.coneCosine = 128;
    assert(hackLib_nearestTarget(&query) == &actors[1]);
    query.exclude = &actors[1];
    assert(hackLib_nearestTarget(&query) == NULL);
    query.radius = 65535;
    assert(hackLib_nextTarget(&query, NULL) == NULL);
}

static void pulse(HackLibTimedEffect *effect)
{
    assert(hackLib_currentDamageSource()->kind == HACKLIB_DAMAGE_PERIODIC);
    assert(hackLib_currentDamageSource()->abilityRow == effect->abilityRow);
    pulses += effect->stacks;
}

static void removeEffect(HackLibTimedEffect *effect)
{
    assert(effect->target->currentHp >= 0);
    removals++;
}

static const HackLibEffectDef Refresh = {1, 2, 1, HACKLIB_EFFECT_REFRESH, NULL, pulse, removeEffect,
                                         0};
static const HackLibEffectDef Stack = {2, 1, 2, HACKLIB_EFFECT_STACK, NULL, pulse, removeEffect, 0};
static const HackLibEffectDef Replace = {3, 0, 1, HACKLIB_EFFECT_REPLACE, NULL, NULL, removeEffect,
                                         0};

static void checkEffects(void)
{
    HackLibTimedEffect *effect;
    resetActors();
    failAllocation = TRUE;
    assert(hackLib_applyEffect(&Refresh, actors, &actors[1], 4, 10, 7) == NULL);
    assert(actors[1].tickCallback == nativeTick);
    failAllocation = FALSE;
    effect = hackLib_applyEffect(&Refresh, actors, &actors[1], 4, 10, 7);
    assert(effect != NULL);
    actors[1].tickCallback(&actors[1]);
    assert(effect->remaining == 3 && pulses == 0);
    assert(hackLib_applyEffect(&Refresh, actors, &actors[1], 2, 20, 7) == effect);
    assert(effect->remaining == 3 && effect->magnitude == 10);
    actors[1].tickCallback(&actors[1]);
    assert(pulses == 1);
    actors[1].tickCallback(&actors[1]);
    actors[1].tickCallback(&actors[1]);
    assert(removals == 1 && pulses == 2 && actors[1].tickCallback == nativeTick);
    assert(nativeTicks == 4);
    assert(hackLib_currentDamageSource() == NULL);
    effect = hackLib_applyEffect(&Refresh, actors, actors, 2, 10, 7);
    assert(effect != NULL);
    hackLib_clearOwnerEffects(actors);
    effect = hackLib_applyEffect(&Stack, actors, &actors[1], 4, 10, 8);
    hackLib_applyEffect(&Stack, actors, &actors[1], 5, 10, 8);
    hackLib_applyEffect(&Stack, actors, &actors[1], 1, 10, 8);
    assert(effect->stacks == 2 && effect->remaining == 5);
    assert(hackLib_applyEffect(&Stack, &actors[2], &actors[1], 3, 10, 8) != effect);
    hackLib_clearOwnerEffects(actors);
    assert(hackLib_countEffects(actors) == 0 && actors[1].tickCallback != nativeTick);
    hackLib_clearOwnerEffects(&actors[2]);
    assert(actors[1].tickCallback == nativeTick);
    effect = hackLib_applyEffect(&Replace, actors, &actors[1], 4, 10, 8);
    assert(hackLib_applyEffect(&Replace, actors, &actors[1], 2, 20, 8) == effect);
    assert(effect->remaining == 2 && effect->magnitude == 20);
    // Reusing the address with a new native callback must discard the stale attachment.
    actors[1].tickCallback = nativeTick;
    assert(hackLib_effectPoolUsed() == 0);
    for (u32 i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        assert(hackLib_applyEffect(&Refresh, actors, &actors[i + 1], 4, 10, 7));
    }
    assert(hackLib_applyEffect(&Refresh, actors, &actors[20], 4, 10, 7) == NULL);
    hackLib_clearOwnerEffects(actors);
    assert(hackLib_effectPoolUsed() == 0);
    assert(hackLib_applyEffect(&Refresh, actors, &actors[1], 4, 10, 7));
    u32 previousRemovals = removals;
    hackLib_noteActorRetired(&actors[1]);
    hackLib_noteActorAllocated(&actors[1]);
    actors[1].tickCallback = nativeTick;
    assert(hackLib_effectPoolUsed() == 0);
    assert(removals == previousRemovals && actors[1].tickCallback == nativeTick);
    assert(hackLib_applyEffect(&Refresh, actors, &actors[1], 4, 10, 7));
    hackLib_noteActorRetired(actors);
    hackLib_noteActorAllocated(actors);
    assert(hackLib_countEffects(actors) == 0);
    assert(actors[1].tickCallback == nativeTick);
}

bool32 skillExpansion_isFoe(Actor *actor)
{
    return actor != actors && actor != NULL && actor->currentHp > 0;
}
static u8 lastElement;
static u32 statusDamage;
HackLibDamageResult skillExpansion_dealElementalDamage(Actor *owner, Actor *target, u16 damage,
                                                       u8 kind, u8 row, u8 elements)
{
    assert(owner == actors && kind == HACKLIB_DAMAGE_PERIODIC && row == 7);
    lastElement = elements;
    statusDamage += damage;
    s32 before = target->currentHp;
    target->currentHp -= damage;
    return hackLib_damageResult(before, target->currentHp, TRUE);
}
static void checkStatuses(void)
{
    resetActors();
    statusDamage = 0;
    assert(skillStatus_apply(SKILL_STATUS_POISON, actors, &actors[1], 60, 2, 7));
    for (u32 i = 0; i < 4; i++)
    {
        assert(skillStatus_apply(SKILL_STATUS_POISON, actors, &actors[1], 60, 9, 7));
    }
    for (u32 i = 0; i < 30; i++)
    {
        actors[1].tickCallback(&actors[1]);
    }
    assert(statusDamage == 6 && lastElement == HACKLIB_ELEMENT_POISON);
    assert(skillStatus_apply(SKILL_STATUS_BURN, actors, &actors[1], 15, 3, 7));
    for (u32 i = 0; i < 15; i++)
    {
        actors[1].tickCallback(&actors[1]);
    }
    assert(statusDamage == 9 && lastElement == HACKLIB_ELEMENT_FIRE);
    assert(skillStatus_clear(&actors[1], SKILL_STATUS_POISON) == 1);
    assert(skillStatus_clear(&actors[1], SKILL_STATUS_POISON) == 0);
    actors[1].coords.x = 0;
    assert(skillStatus_apply(SKILL_STATUS_SLOW, actors, &actors[1], 2, 0, 7));
    actors[1].tickCallback(&actors[1]);
    actors[1].tickCallback(&actors[1]);
    assert(actors[1].coords.x == 4 * FP16_16_ONE);
    actors[1].tickCallback(&actors[1]);
    assert(actors[1].coords.x == 8 * FP16_16_ONE && actors[1].tickCallback == nativeTick);
    assert(skillStatus_apply(SKILL_STATUS_SLOW, actors, &actors[1], 20, 0, 7));
    assert(skillStatus_apply(SKILL_STATUS_SLOW, &actors[2], &actors[1], 20, 0, 7));
    s32 beforeSlow = actors[1].coords.x;
    actors[1].tickCallback(&actors[1]);
    assert(actors[1].coords.x - beforeSlow == 2 * FP16_16_ONE);
    assert(skillStatus_clear(&actors[1], SKILL_STATUS_SLOW) == 2);
    assert(actors[1].tickCallback == nativeTick);
    assert(!skillStatus_apply(SKILL_STATUS_BURN, actors, &actors[1], 15, 21846, 7));
    assert(!skillStatus_apply(SKILL_STATUS_POISON, actors, &actors[1], 15, 0, 7));
    assert(!skillStatus_apply(SKILL_STATUS_SLOW, actors, actors, 15, 0, 7));
    failAllocation = TRUE;
    assert(!skillStatus_apply(SKILL_STATUS_SLOW, actors, &actors[1], 15, 0, 7));
    failAllocation = FALSE;
    assert(skillStatus_apply(SKILL_STATUS_BURN, actors, &actors[1], 90, 3, 7));
    actors[1].currentHp = 0;
    actors[1].tickCallback(&actors[1]);
    assert(hackLib_effectPoolUsed() == 0 && actors[1].tickCallback == nativeTick);
}

static Actor *spawn(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime, u16 damage)
{
    (void)owner;
    (void)position;
    (void)target;
    (void)lifetime;
    (void)damage;
    return &actors[1 + spawns++];
}

static void despawn(Actor *actor)
{
    despawns++;
    actor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
}

static const HackLibSummonDef Summon = {
    .spawn = spawn, .despawn = despawn, .managedLifetime = TRUE};

static void checkSummons(void)
{
    Vector2Fp16 position = {0};
    Actor *summon;
    resetActors();
    failAllocation = TRUE;
    assert(hackLib_spawnSummon(&Summon, actors, &position, NULL, 4, 30, 1, 7) == NULL);
    assert(spawns == 0);
    failAllocation = FALSE;
    summon = hackLib_spawnSummon(&Summon, actors, &position, NULL, 4, 30, 1, 7);
    assert(summon != NULL);
    assert(hackLib_spawnSummon(&Summon, actors, &position, NULL, 4, 30, 1, 7) == NULL);
    // Effects can wrap a summon without losing its registration or native callback.
    assert(hackLib_applyEffect(&Refresh, actors, summon, 10, 1, 7));
    assert(hackLib_countSummons(actors, &Summon) == 1);
    summon->tickCallback(summon);
    summon->tickCallback(summon);
    summon->tickCallback(summon);
    assert(despawns == 1 && hackLib_countSummons(actors, &Summon) == 0);
    hackLib_clearOwnerEffects(actors);
    for (u32 i = 0; i < HACKLIB_SUMMON_LIMIT; i++)
    {
        assert(hackLib_spawnSummon(&Summon, actors, &position, NULL, 10, 30, 20, 7));
    }
    assert(hackLib_spawnSummon(&Summon, actors, &position, NULL, 10, 30, 20, 7) == NULL);
    hackLib_clearOwnerSummons(actors);
    assert(despawns == 1 + HACKLIB_SUMMON_LIMIT && hackLib_summonPoolUsed() == 0);
    hackLib_clearOwnerSummons(actors);
    assert(despawns == 1 + HACKLIB_SUMMON_LIMIT);
    summon = hackLib_spawnSummon(&Summon, actors, &position, NULL, 10, 30, 20, 7);
    assert(summon);
    hackLib_noteActorRetired(actors);
    hackLib_noteActorAllocated(actors);
    assert(hackLib_countSummons(actors, &Summon) == 0);
    assert(despawns == 2 + HACKLIB_SUMMON_LIMIT);
}

static void stamp(SkillSaveRecord *record)
{
    record->sum = 0;
    record->sum = -skillExpansion_saveByteSum(record);
}

static void checkSaves(void)
{
    SkillSaveRecord record = {
        .tag = SKILL_SAVE_TAG, .version = SKILL_SAVE_VERSION, .heroId = 8, .slotChecksum = 1234};
    assert(sizeof(record) == 144);
    record.actives[0] = (SkillSaveEntry){0x908, 4, 1};
    record.actives[1] = (SkillSaveEntry){0x901, 2, 0};
    record.quickSelect[0] = 0x908;
    stamp(&record);
    assert(skillExpansion_isValidSave(&record, 8, 1234));
    assert(!skillExpansion_isValidSave(&record, 7, 1234));
    assert(!skillExpansion_isValidSave(&record, 8, 1235));
    // Identity lookup survives a different row order.
    assert(skillExpansion_findSaveEntry(record.actives, 12, 0x901)->level == 2);
    assert(skillExpansion_findSaveEntry(record.actives, 12, 0) == NULL);
    assert(skillExpansion_findSaveEntry(record.actives, 12, 0x999) == NULL);
    record.actives[2] = record.actives[0];
    stamp(&record);
    assert(!skillExpansion_isValidSave(&record, 8, 1234));
    memset(&record.actives[2], 0, sizeof(record.actives[2]));
    record.version++;
    stamp(&record);
    assert(!skillExpansion_isValidSave(&record, 8, 1234));
    record.version = SKILL_SAVE_VERSION;
    record.actives[0].level = 6;
    stamp(&record);
    assert(!skillExpansion_isValidSave(&record, 8, 1234));
    record.actives[0].level = 4;
    stamp(&record);
    record.sum++;
    assert(!skillExpansion_isValidSave(&record, 8, 1234));
}

int main(void)
{
    resetActors();
    HackLibActorRef old = hackLib_actorRef(&actors[0]);
    assert(hackLib_isActorRefValid(old));
    hackLib_noteActorRetired(&actors[0]);
    assert(!hackLib_isActorRefValid(old));
    hackLib_noteActorAllocated(&actors[0]);
    assert(!hackLib_isActorRefValid(old));
    assert(hackLib_isActorRefValid(hackLib_actorRef(&actors[0])));
    assert(hackLib_enableProfiler(TRUE));
    HackLibProfileSpan span = hackLib_profileBegin(HACKLIB_PROFILE_TARGETS);
    profileClock = 12;
    hackLib_profileWork(HACKLIB_PROFILE_TARGETS, 3);
    hackLib_profileEnd(span);
    HackLibProfileRecord record;
    assert(hackLib_getProfile(HACKLIB_PROFILE_TARGETS, &record));
    assert(record.calls == 1 && record.peakLines == 12 && record.work == 3);
    hackLib_resetProfiler();
    assert(hackLib_getProfile(HACKLIB_PROFILE_TARGETS, &record) && record.calls == 0);
    assert(hackLib_enableProfiler(FALSE));
    checkTargeting();
    checkEffects();
    checkStatuses();
    checkSummons();
    checkSaves();
    puts("PASS: targeting, effect lifecycle, summon pools and stable save identities");
    return 0;
}
