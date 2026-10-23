#include "patches/skill_expansion/skill_delivery.h"
#include "game.h"
#include "lib/profile.h"
#include "lib/skill_fx.h"
#include "lib/targeting.h"
#include "patches/debug_cheats/delivery_preview.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_status.h"
#include "scene.h"
#include "stats.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PlayerGlobals g_PlayerGlobals;
PlayerHud g_PlayerHuds[2];
GameFlags g_GameFlags;
SceneState g_SceneCurrent;
const Vector2Fp16 Direction32MoveVectors[32] = {
    [8] = {65536, 0}, [0] = {0, -65536}, [4] = {46340, -46340}};
static Actor owner, enemies[10];
static u32 generations[10], ownerGeneration = 1, allocations, hits, sounds, visuals, queries;
static s32 failAt = -1, wall = -1;
static bool miss, clearOnHit, retireOnHit;
static u8 lastRow;

void *memory_zalloc(size_t size)
{
    if (failAt == 0)
    {
        return NULL;
    }
    if (failAt > 0)
    {
        failAt--;
    }
    allocations++;
    return calloc(1, size);
}
void memory_free(void *ptr)
{
    assert(allocations);
    allocations--;
    free(ptr);
}
HackLibActorRef hackLib_actorRef(Actor *actor)
{
    if (actor == &owner)
    {
        return (HackLibActorRef){actor, ownerGeneration};
    }
    for (u32 i = 0; i < 10; i++)
    {
        if (actor == &enemies[i])
        {
            return (HackLibActorRef){actor, generations[i]};
        }
    }
    return (HackLibActorRef){0};
}
bool hackLib_isActorRefValid(HackLibActorRef ref)
{
    return ref.actor && ref.generation && hackLib_actorRef(ref.actor).generation == ref.generation;
}
s32 hackLib_actorCurrentHp(const Actor *actor)
{
    return actor ? actor->currentHp : 0;
}
bool32 skillExpansion_isPlayerAvatar(Actor *actor)
{
    return actor == &owner;
}
bool32 skillExpansion_isFoe(Actor *actor)
{
    return actor != &owner && actor && actor->currentHp > 0 &&
           !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}
HackLibDamageResult skillExpansion_dealDamage(Actor *attacker, Actor *target, u16 damage, u8 kind,
                                              u8 row)
{
    assert(attacker == &owner && kind == HACKLIB_DAMAGE_ABILITY);
    lastRow = row;
    hits++;
    if (clearOnHit)
    {
        skillDelivery_clear(0);
    }
    if (retireOnHit)
    {
        generations[target - enemies]++;
    }
    if (miss)
    {
        return (HackLibDamageResult){0};
    }
    s32 before = target->currentHp;
    target->currentHp -= damage;
    return hackLib_damageResult(before, target->currentHp, TRUE);
}
static u8 lastElements, appliedStatus;
HackLibDamageResult skillExpansion_dealElementalDamage(Actor *attacker, Actor *target, u16 damage,
                                                       u8 kind, u8 row, u8 elements)
{
    lastElements = elements;
    return skillExpansion_dealDamage(attacker, target, damage, kind, row);
}
bool skillStatus_apply(u8 kind, Actor *attacker, Actor *target, u16 frames, u16 power, u8 row)
{
    assert(attacker == &owner && target != &owner && frames == 90 && power == 2 && row == 7);
    appliedStatus = kind;
    return TRUE;
}
ActorList g_ActorListMain;
HackLibProfileSpan hackLib_profileBegin(u8 region)
{
    (void)region;
    return (HackLibProfileSpan){0};
}
void hackLib_profileEnd(HackLibProfileSpan span)
{
    (void)span;
}
void hackLib_profileWork(u8 region, u32 count)
{
    (void)region;
    queries += count;
}
Actor *actor_iterateInRadius(Actor *center, Actor *cursor, s32 radius)
{
    (void)center;
    (void)cursor;
    (void)radius;
    return NULL;
}

void hackLib_stopEffect(HackLibEffectHandle *handle)
{
    memset(handle, 0, sizeof(*handle));
}
u32 hackLib_playEffect(u16 id, Actor *on, const Vector2Fp16 *position, HackLibEffectHandle *handle)
{
    (void)id;
    (void)on;
    (void)position;
    (void)handle;
    visuals++;
    return 0;
}
void hackLib_fxDetach(Actor *actor, const Vector2Fp16 *position)
{
    (void)actor;
    (void)position;
}
void terrain_getWorldSize(Vector2Int *size)
{
    *size = (Vector2Int){512, 512};
}
u8 terrain_getCollisionAt(s32 x, s32 y)
{
    (void)y;
    return x == wall ? 1 : 0;
}
bool hackLib_canLandAt(const Actor *actor, const Vector2Fp16 *position)
{
    (void)actor;
    return (position->x >> 16) != wall;
}
bool actor_isStunnable(Actor *actor)
{
    (void)actor;
    return TRUE;
}
Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 flag)
{
    (void)parent;
    (void)target;
    (void)duration;
    (void)kind;
    (void)arg4;
    (void)arg5;
    (void)flag;
    return NULL;
}
u32 sfx_play(s32 id)
{
    (void)id;
    sounds++;
    return 0;
}
void skillExpansion_feedback(u8 p, const char *message)
{
    (void)p;
    (void)message;
}
char *text_formatInt(s32 value, char *output)
{
    sprintf(output, "%d", value);
    return output;
}

static SkillDeliveryDef def;
static void target(u32 index, s32 x, s32 y)
{
    enemies[index].currentHp = 1000;
    enemies[index].coords = (Vector2Fp16){x * 65536, y * 65536};
}
static void reset(void)
{
    skillDelivery_clear(0);
    skillDelivery_clear(1);
    assert(!allocations);
    memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    memset(&owner, 0, sizeof(owner));
    memset(enemies, 0, sizeof(enemies));
    owner.coords = (Vector2Fp16){100 * 65536, 100 * 65536};
    owner.currentHp = 100;
    owner.facing = 2;
    PLAYER(0).ownerActor = &owner;
    for (u32 i = 0; i < 10; i++)
    {
        generations[i] = 1;
    }
    g_ActorListMain.head = &enemies[0];
    for (u32 i = 0; i < 9; i++)
    {
        enemies[i].next = &enemies[i + 1];
    }
    ownerGeneration = 1;
    g_SceneCurrent.id = SCENE_ID_GAMEPLAY;
    g_GameFlags.p = GAME_FLAG_BONUS_MAP_ACTIVE;
    hits = sounds = visuals = queries = 0;
    lastElements = appliedStatus = 0;
    failAt = wall = -1;
    miss = clearOnHit = retireOnHit = FALSE;
    def = SkillDeliveryDefinitions[0];
    def.travelEffect = def.trailEffect = def.impactEffect = SKILL_DELIVERY_NO_EFFECT;
    def.speed = 16;
    def.radius = 1;
}
static bool launch(bool cosmetic)
{
    return skillDelivery_launch(0, &def, &owner.coords, 8, 1, 7, cosmetic);
}
static void ticks(u32 n)
{
    while (n--)
    {
        skillDelivery_tick(0, &owner);
    }
}
static SkillDeliveryStats stats(void)
{
    SkillDeliveryStats result;
    skillDelivery_getStats(0, &result);
    return result;
}
int main(void)
{
    reset();
    target(0, 108, 100);
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1 && enemies[0].currentHp == 992 && lastRow == 7 && !stats().active);
    assert(queries == 10);
    ticks(50);
    assert(hits == 1);

    reset();
    target(0, 110, 100);
    target(1, 105, 100);
    target(2, 114, 100);
    def.pierceTargets = 2;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 2 && enemies[1].currentHp == 992 && enemies[0].currentHp == 992 &&
           enemies[2].currentHp == 1000);

    reset();
    target(0, 108, 100);
    target(1, 108, 110);
    target(2, 108, 150);
    def.splashRadius = 16;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 2 && stats().contacts == 2 && enemies[2].currentHp == 1000);

    reset();
    target(0, 108, 100);
    wall = 104;
    assert(launch(FALSE));
    ticks(1);
    assert(!hits && !stats().active);
    reset();
    target(0, 108, 100);
    wall = 104;
    def.stopOnWalls = FALSE;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1);

    reset();
    target(0, 108, 100);
    miss = TRUE;
    assert(launch(FALSE));
    ticks(10);
    assert(hits == 1 && stats().hits == 0 && stats().contacts == 1 && enemies[0].currentHp == 1000);

    reset();
    target(0, 108, 100);
    def.stunTicks = 30;
    def.knockback = 8;
    assert(launch(TRUE));
    ticks(1);
    assert(!hits && stats().contacts == 1 && enemies[0].actionState != ACTOR_STATE_STUNNED);
    reset();
    target(0, 108, 100);
    def.stunTicks = 30;
    def.knockback = 8;
    wall = 112;
    assert(launch(FALSE));
    ticks(1);
    assert(enemies[0].actionState == ACTOR_STATE_STUNNED && enemies[0].coords.x == 111 * 65536);

    reset();
    target(0, 105, 100);
    def = SkillDeliveryDefinitions[4];
    assert(launch(FALSE));
    ticks(46);
    assert(hits == 3 && stats().contacts == 3 && stats().damage == 24 && !stats().active);
    reset();
    for (u32 i = 0; i < 10; i++)
    {
        target(i, 105, 100);
    }
    def = SkillDeliveryDefinitions[3];
    def.maxTargets = 3;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 3);

    reset();
    target(0, 108, 100);
    assert(launch(FALSE));
    g_GameFlags.d.overlay = TRUE;
    ticks(30);
    assert(!hits && stats().active == 1);
    g_GameFlags.d.overlay = FALSE;
    ticks(1);
    assert(hits == 1);

    reset();
    target(0, 108, 100);
    assert(launch(FALSE));
    ownerGeneration++;
    ticks(1);
    assert(!hits && !allocations);
    reset();
    target(0, 108, 100);
    clearOnHit = TRUE;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1 && !allocations);
    reset();
    target(0, 108, 100);
    retireOnHit = TRUE;
    def.stunTicks = 10;
    assert(launch(FALSE));
    ticks(1);
    assert(enemies[0].actionState != ACTOR_STATE_STUNNED);

    reset();
    def.lifetime = 2;
    for (u32 i = 0; i < SKILL_DELIVERY_LIMIT; i++)
    {
        assert(launch(FALSE));
    }
    assert(!launch(FALSE));
    ticks(2);
    assert(stats().active == 0 && allocations == 1);
    reset();
    failAt = 0;
    assert(!launch(FALSE) && !allocations);
    reset();
    failAt = 1;
    assert(!launch(FALSE) && !allocations);
    reset();
    def.speed = 0;
    assert(!launch(FALSE));

    reset();
    target(0, 108, 100);
    DeliveryPreviewOptions[0].set(0);
    DeliveryPreviewOptions[4].set(1);
    deliveryPreview_playerTick(0);
    ticks(3);
    assert(!hits && stats().contacts == 1);
    reset();
    target(0, 108, 100);
    g_GameFlags.p |= GAME_FLAG_COOP_ACTIVE;
    DeliveryPreviewOptions[4].set(1);
    deliveryPreview_playerTick(0);
    assert(!allocations);
    reset();
    target(0, 108, 100);
    target(1, 108, 110);
    def.splashRadius = 16;
    def.maxTargets = 1;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1);
    reset();
    target(0, 108, 100);
    target(1, 108, 110);
    def.splashRadius = 16;
    clearOnHit = TRUE;
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1 && !allocations);
    reset();
    target(0, 105, 100);
    def = SkillDeliveryDefinitions[4];
    assert(launch(FALSE));
    ticks(1);
    generations[0]++;
    ticks(15);
    assert(hits == 2);
    skillDelivery_resetStats(0);
    assert(stats().hits == 0 && stats().active == 1);
    reset();
    target(0, 108, 100);
    enemies[0].flags.p |= ACTOR_FLAG_PENDING_REMOVE;
    target(1, 112, 100);
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1 && enemies[0].currentHp == 1000 && enemies[1].currentHp == 992);
    reset();
    target(0, 108, 100);
    owner.currentHp = 0;
    assert(!launch(FALSE));
    reset();
    g_SceneCurrent.id = 0;
    assert(launch(FALSE));
    ticks(1);
    assert(!allocations);
    reset();
    target(0, 108, 100);
    def = SkillDeliveryDefinitions[SKILL_DELIVERY_TEST_VENOM];
    assert(launch(FALSE));
    assert(!launch(FALSE));
    ticks(1);
    assert(lastElements == HACKLIB_ELEMENT_POISON && appliedStatus == SKILL_STATUS_POISON);
    g_GameFlags.d.overlay = TRUE;
    ticks(30);
    assert(!launch(FALSE));
    g_GameFlags.d.overlay = FALSE;
    ticks(29);
    assert(launch(FALSE));
    reset();
    target(0, 108, 100);
    def = SkillDeliveryDefinitions[SKILL_DELIVERY_TEST_VENOM];
    miss = TRUE;
    assert(launch(FALSE));
    ticks(1);
    assert(!appliedStatus);
    reset();
    target(0, 108, 100);
    def = SkillDeliveryDefinitions[SKILL_DELIVERY_TEST_VENOM];
    assert(launch(TRUE));
    assert(launch(FALSE));
    ticks(1);
    assert(hits == 1 && appliedStatus == SKILL_STATUS_POISON);
    reset();
    puts("PASS: delivery collisions, piercing, splash, repeats, walls, status, preview and "
         "lifecycle");
    return 0;
}
