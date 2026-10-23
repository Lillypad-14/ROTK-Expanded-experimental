#include "lib/actor_ref.h"
#define asm(...)
#include "patches/skill_expansion/skill_expansion_elite.c"
#undef asm
#include <assert.h>
#include <stdio.h>
#include <string.h>
PlayerGlobals g_PlayerGlobals;
GameFlags g_GameFlags;
const EnemyTypeInfo EnemyTypeInfos[83] = {
    [ENEMY_TYPE_ORC_ARCHER_FIRE] = {.attackFlags.p = ENEMY_ATTACK_RANGED | ENEMY_ATTACK_FIRE},
    [ENEMY_TYPE_ORC_ARCHER_POISON] = {.attackFlags.p = ENEMY_ATTACK_RANGED | ENEMY_ATTACK_POISON,
                                      .poisonPower = 3}};
void player_addHp(u8 p, s16 amount, bool flinch)
{
    skillExpansion_addHp(p, amount, flinch);
}
SkillExpansionPlayer skillExpansion_players[2];
static Actor avatar, foe;
static u32 generation = 1;
void *memory_memset(void *p, u32 value, size_t count)
{
    return memset(p, value, count);
}
s32 skillExpansion_getPassiveLevel(u8 p, u8 id);
Actor *hackLib_nearestTarget(const HackLibTargetQuery *query)
{
    (void)query;
    return &foe;
}
HackLibActorRef hackLib_actorRef(Actor *actor)
{
    return (HackLibActorRef){actor, generation};
}
u32 hackLib_actorGeneration(Actor *actor)
{
    (void)actor;
    return generation;
}
bool hackLib_isActorRefValid(HackLibActorRef ref)
{
    return ref.actor && ref.generation == generation;
}
Actor *hackLib_fxBurst(Actor *a, u8 shape, HackLibFxColor color, u8 layer)
{
    (void)a;
    (void)shape;
    (void)color;
    (void)layer;
    return NULL;
}
u32 sfx_play(s32 id)
{
    (void)id;
    return 0;
}

ActorList g_ActorListMain;
const HackLibHudMeter HackLibHudGapMeter = {0};
static u8 passiveLevels[SKILL_EXPANSION_PASSIVE_END];
static const HackLibDamageSource *source;
s32 skillExpansion_getPassiveLevel(u8 p, u8 id)
{
    (void)p;
    return passiveLevels[id];
}
static bool nativeHit = TRUE;
static bool nativeIncomingHit, nativeCritical, nativeAbsorb, nativeRetaliate;
static bool retaliateFromEvent, healFromEvent;
static bool healOnOutgoingHit, healInsideHpChange, nestedIncoming;
static s32 incomingReduction;
static u32 incomingAdjustments, hpChanges;
static bool eventKitEnabled;
static char eventOrder[16];
static u32 eventCount;
static HackLibCombatEvent lastEvent;
static HackLibCombatEvent lastIncoming;
static void eventIncoming(u8 playerIndex, const HackLibCombatEvent *event)
{
    assert(event->victim == PLAYER(playerIndex).ownerActor && event->attacker == &foe);
    const HackLibDamageSource *parent = hackLib_currentDamageSource();
    assert(parent->origin == &foe);
    lastIncoming = *event;
    eventOrder[eventCount++] = 'I';
    if (retaliateFromEvent)
    {
        skillExpansion_dealDamage(event->victim, event->attacker, 7, HACKLIB_DAMAGE_SECONDARY, 9);
        assert(hackLib_currentDamageSource() == parent);
    }
    if (healFromEvent)
    {
        skillExpansion_addHp(playerIndex, 10, FALSE);
    }
}
static s32 eventIncomingDamage(u8 playerIndex, Actor *attacker, s32 damage)
{
    (void)playerIndex;
    assert(attacker == &foe);
    incomingAdjustments++;
    return damage - incomingReduction;
}
static void eventHpChange(u32 playerIndex, s32 amount, u32 flinch)
{
    hpChanges++;
    skillExpansion_original_player_addHp(playerIndex, amount, flinch);
    if (amount < 0 && healInsideHpChange)
    {
        skillExpansion_addHp(playerIndex, 5, FALSE);
    }
    if (amount < 0 && nestedIncoming)
    {
        nestedIncoming = FALSE;
        skillExpansion_resolveEnemyAttackOnPlayer(&foe, PLAYER(playerIndex).ownerActor);
    }
}
static void eventKill(u8 playerIndex, Actor *attacker, Actor *victim)
{
    assert(playerIndex == 0 && attacker == &avatar && victim == &foe);
    eventOrder[eventCount++] = 'K';
}
static void eventHit(u8 playerIndex, Actor *attacker, Actor *victim, HackLibDamageResult result,
                     bool melee)
{
    (void)melee;
    assert(playerIndex == 0 && attacker == &avatar && victim == &foe && result.hit);
    eventOrder[eventCount++] = 'H';
    if (healOnOutgoingHit)
    {
        skillExpansion_addHp(playerIndex, 5, FALSE);
    }
}
static void eventResolved(u8 playerIndex, const HackLibCombatEvent *event)
{
    assert(playerIndex == 0);
    assert(hackLib_currentDamageSource()->kind == event->source.kind);
    lastEvent = *event;
    eventOrder[eventCount++] = 'R';
}
static const SkillExpansionHeroKit EventKit = {.onKill = eventKill,
                                               .onHit = eventHit,
                                               .onCombatEvent = eventResolved,
                                               .onIncomingCombatEvent = eventIncoming,
                                               .incomingDamage = eventIncomingDamage,
                                               .onHpChange = eventHpChange};
const SkillExpansionHeroKit *skillExpansion_getHeroKit(u32 id)
{
    (void)id;
    return eventKitEnabled ? &EventKit : NULL;
}
const HackLibDamageSource *hackLib_currentDamageSource(void)
{
    return source;
}
const HackLibDamageSource *hackLib_swapDamageSource(const HackLibDamageSource *next)
{
    const HackLibDamageSource *old = source;
    source = next;
    return old;
}
void skillExpansion_original_player_addHp(u32 p, s32 amount, u32 flinch)
{
    (void)flinch;
    PLAYER(p).currentHp += amount;
}
Actor *fx_spawnOnActor(Actor *a, u8 kind, u8 variant, u8 layer)
{
    (void)a;
    (void)kind;
    (void)variant;
    (void)layer;
    return NULL;
}
void player_setHandFx(u8 p, u8 hand, Item item)
{
    (void)p;
    (void)hand;
    (void)item;
}
u32 fx_tickAttachment(Actor *a)
{
    (void)a;
    return 0;
}
#define HERO_STUBS(hero)                                                                           \
    void skillExpansion_##hero##Reset(u8 p)                                                        \
    {                                                                                              \
        (void)p;                                                                                   \
    }                                                                                              \
    void skillExpansion_##hero##Cleanup(u8 p, Actor *a)                                            \
    {                                                                                              \
        (void)p;                                                                                   \
        (void)a;                                                                                   \
    }                                                                                              \
    void skillExpansion_##hero##Tick(u8 p, Actor *a)                                               \
    {                                                                                              \
        (void)p;                                                                                   \
        (void)a;                                                                                   \
    }                                                                                              \
    s32 skillExpansion_##hero##Damage(u8 p, s32 damage, bool melee)                                \
    {                                                                                              \
        (void)p;                                                                                   \
        (void)melee;                                                                               \
        return damage;                                                                             \
    }
HERO_STUBS(frodo)
HERO_STUBS(smeagol)
HERO_STUBS(sam)
s32 skillExpansion_frodoIncomingDamage(u8 p, Actor *a, s32 damage)
{
    (void)p;
    (void)a;
    return damage;
}
s32 skillExpansion_samIncomingDamage(u8 p, Actor *a, s32 damage)
{
    (void)p;
    (void)a;
    return damage;
}
s32 skillExpansion_smeagolIncomingDamage(u8 p, s32 damage)
{
    (void)p;
    return damage;
}
void skillExpansion_gimliReset(u8 p)
{
    (void)p;
}
void skillExpansion_gimliCleanup(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
void skillExpansion_gimliTick(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
s32 skillExpansion_gimliDamage(u8 p, Actor *a, s32 damage)
{
    (void)p;
    (void)a;
    return damage;
}
s32 skillExpansion_gimliIncomingDamage(u8 p, s32 damage)
{
    (void)p;
    return damage;
}
bool skillExpansion_gimliFlinch(u8 p, bool flinchOnDamage)
{
    (void)p;
    return flinchOnDamage;
}
void skillExpansion_gimliOnKill(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
void skillExpansion_gandalfReset(u8 p)
{
    (void)p;
}
void skillExpansion_gandalfCleanup(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
void skillExpansion_gandalfTick(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
s32 skillExpansion_gandalfDamage(u8 p, Actor *a, s32 damage, bool melee)
{
    (void)p;
    (void)a;
    (void)melee;
    return damage;
}
s32 skillExpansion_gandalfIncomingDamage(u8 p, Actor *a, s32 damage)
{
    (void)p;
    (void)a;
    return damage;
}
void skillExpansion_gandalfOnKill(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
void skillExpansion_legolasReset(u8 p)
{
    (void)p;
}
void skillExpansion_legolasCleanup(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
void skillExpansion_legolasTick(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
s32 skillExpansion_legolasDamage(u8 p, Actor *a, s32 damage, bool melee)
{
    (void)p;
    (void)a;
    (void)melee;
    return damage;
}
s32 skillExpansion_legolasIncomingDamage(u8 p, s32 damage)
{
    (void)p;
    return damage;
}
bool skillExpansion_legolasFlinch(u8 p, bool flinchOnDamage)
{
    (void)p;
    return flinchOnDamage;
}
void skillExpansion_frodoOnHit(u8 p, Actor *a, HackLibDamageResult result, bool melee)
{
    (void)p;
    (void)a;
    (void)result;
    (void)melee;
}
void skillExpansion_smeagolOnHit(u8 p, Actor *a, HackLibDamageResult result, bool melee)
{
    (void)p;
    (void)a;
    (void)result;
    (void)melee;
}
void skillExpansion_frodoWrapStockCallbacks(u8 p, ActiveSkill *rows)
{
    (void)p;
    (void)rows;
}
s32 skillExpansion_original_combat_computePveDamage(Actor *a, Actor *b, bool crit)
{
    (void)a;
    (void)b;
    (void)crit;
    return 10;
}
bool skillExpansion_original_combat_resolvePveAttack(Actor *a, Actor *b)
{
    if (nativeHit && !nativeAbsorb)
    {
        b->currentHp -= skillExpansion_computePveDamage(a, b, nativeCritical);
    }
    return nativeHit;
}
bool skillExpansion_original_combat_applyPveRangedHit(Actor *a, Actor *b, u16 damage)
{
    (void)a;
    if (nativeHit)
    {
        b->currentHp -= damage;
    }
    return nativeHit;
}
bool skillExpansion_original_combat_resolveEnemyAttackOnPlayer(Actor *a, Actor *b)
{
    if (nativeRetaliate)
    {
        skillExpansion_resolvePveAttack(b, a);
    }
    if (nativeIncomingHit)
    {
        u32 sfx = 0;
        skillExpansion_handleEnemyStatusArmor(a, b, &sfx, 10);
    }
    if (nativeIncomingHit && !nativeAbsorb)
    {
        skillExpansion_addHp(b->as.combat.playerIndex, -10, FALSE);
    }
    return nativeIncomingHit;
}
u16 skillExpansion_original_enemyStatusArmor(Actor *a, Actor *b, u32 *sfx, s16 rawDamage)
{
    u8 p = b->as.combat.playerIndex;
    Actor *shield = PLAYER(p).shieldBubble;
    assert(rawDamage == 10 && sfx != NULL);
    if ((EnemyTypeInfos[a->unitType].attackFlags.p & ENEMY_ATTACK_POISON) &&
        !(PLAYER(p).heroId == HERO_ID_GANDALF && shield != NULL &&
          shield->as.shieldGandalf.shieldHealth >= rawDamage))
    {
        b->as.combat.venomDuration = 90 - 90 * PLAYER_STAT(p, STAT_POISON_RESIST_PERCENT) / 100;
        skillExpansion_addHp(p, -2, FALSE);
    }
    return 0;
}
void skillExpansion_original_actor_player_tick(Actor *a)
{
    (void)a;
}
void skillExpansion_runtimeTick(u8 p, Actor *a)
{
    (void)p;
    (void)a;
}
HackLibProfileSpan hackLib_profileBegin(u8 region)
{
    (void)region;
    return (HackLibProfileSpan){0};
}
void hackLib_profileEnd(HackLibProfileSpan span)
{
    (void)span;
}
void skillExpansion_addTemporaryLevels(u8 p, u8 count)
{
    for (u32 i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
    {
        *skillExpansion_activeLevel(p, i) += count;
    }
}
void skillExpansion_removeTemporaryLevels(u8 p, u8 count)
{
    for (u32 i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
    {
        *skillExpansion_activeLevel(p, i) -= count;
    }
}
void actor_npcGhost_trigger(Actor *a, Actor *b)
{
    (void)a;
    (void)b;
}
#define STOCK_CAST(name)                                                                           \
    void skill_active_##name(Actor *a)                                                             \
    {                                                                                              \
        (void)a;                                                                                   \
    }
u32 skill_active_aragornSweep(Actor *actor)
{
    (void)actor;
    return 0;
}
STOCK_CAST(aragornKingsCommand)
STOCK_CAST(aragornNumenoreanWill)
STOCK_CAST(aragornCallOfTheDead)
STOCK_CAST(herbalHealing)
Actor *actor_iterateInRadius(Actor *a, Actor *cursor, s32 radius)
{
    (void)a;
    (void)cursor;
    (void)radius;
    return NULL;
}
s32 rng_rollRange(s32 low, s32 high)
{
    (void)high;
    return low;
}
s16 skill_active_getLeveledValue(u8 p, u8 row, u8 value, u8 level)
{
    (void)p;
    (void)row;
    (void)value;
    return level;
}
void hackLib_drawHudMeter(const HackLibHudMeter *geometry, s32 value, s32 max,
                          const HackLibHudMeterLook *look, const u8 *marks, u32 count)
{
    (void)geometry;
    (void)value;
    (void)max;
    (void)look;
    (void)marks;
    (void)count;
}
bool actor_isStunnable(Actor *a)
{
    (void)a;
    return TRUE;
}
Actor *fx_attachStatusEffect(Actor *a, Actor *b, u16 duration, u8 kind, u8 arg4, u8 arg5, u8 flag)
{
    (void)a;
    (void)b;
    (void)duration;
    (void)kind;
    (void)arg4;
    (void)arg5;
    (void)flag;
    return NULL;
}
static void resetFixture(void)
{
    memset(states, 0, sizeof(states));
    memset(&avatar, 0, sizeof(avatar));
    memset(&foe, 0, sizeof(foe));
    memset(passiveLevels, 0, sizeof(passiveLevels));
    memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    memset(skillExpansion_players, 0, sizeof(skillExpansion_players));
    PLAYER(0).heroId = HERO_ID_ARAGORN;
    PLAYER(0).ownerActor = &avatar;
    PLAYER(0).castingActiveSkill = 7;
    PLAYER(0).currentHp = PLAYER_STAT(0, STAT_MAX_HP) = 1000;
    foe.currentHp = 1000;
    foe.flags.p = ACTOR_FLAG_COLLISION_CHECK;
    generation = 1;
    nativeHit = TRUE;
    nativeIncomingHit = nativeCritical = nativeAbsorb = nativeRetaliate = FALSE;
    retaliateFromEvent = healFromEvent = FALSE;
    healOnOutgoingHit = healInsideHpChange = nestedIncoming = FALSE;
    incomingReduction = 0;
    incomingAdjustments = hpChanges = 0;
    eventKitEnabled = FALSE;
    eventCount = 0;
    memset(eventOrder, 0, sizeof(eventOrder));
}
Actor *hackLib_nextTargetAt(const Vector2Fp16 *position, Actor *exclude, u16 radius, Actor *cursor)
{
    (void)position;
    (void)exclude;
    return actor_iterateInRadius(NULL, cursor, (s32)radius * 65536);
}
int main(void)
{
    resetFixture();
    eventKitEnabled = TRUE;
    foe.currentHp = 5;
    assert(skillExpansion_resolvePveAttack(&avatar, &foe));
    assert(strcmp(eventOrder, "KHR") == 0);
    assert(lastEvent.melee && lastEvent.result.damageDealt == 5 && lastEvent.result.killed);
    assert(lastEvent.source.kind == HACKLIB_DAMAGE_WEAPON && source == NULL);
    assert(lastEvent.criticalKnown && !lastEvent.critical);
    assert(lastEvent.source.elementsKnown && lastEvent.source.elements == HACKLIB_ELEMENT_NONE);
    resetFixture();
    eventKitEnabled = TRUE;
    nativeCritical = TRUE;
    assert(skillExpansion_resolvePveAttack(&avatar, &foe));
    assert(lastEvent.criticalKnown && lastEvent.critical);
    resetFixture();
    eventKitEnabled = TRUE;
    HackLibDamageResult ranged =
        skillExpansion_dealDamage(&avatar, &foe, 20, HACKLIB_DAMAGE_PERIODIC, 8);
    assert(strcmp(eventOrder, "HR") == 0 && ranged.damageDealt == 20);
    assert(!lastEvent.melee && lastEvent.source.kind == HACKLIB_DAMAGE_PERIODIC);
    assert(lastEvent.source.abilityRow == 8 && source == NULL);
    resetFixture();
    eventKitEnabled = TRUE;
    nativeHit = FALSE;
    assert(!skillExpansion_resolvePveAttack(&avatar, &foe));
    assert(strcmp(eventOrder, "R") == 0 && !lastEvent.result.hit);
    assert(!lastEvent.criticalKnown && !lastEvent.critical);
    assert(source == NULL);

    resetFixture();
    eventKitEnabled = nativeAbsorb = TRUE;
    assert(skillExpansion_resolvePveAttack(&avatar, &foe));
    assert(lastEvent.result.hit && lastEvent.result.damageDealt == 0);
    assert(!lastEvent.criticalKnown && currentMeleeResolution == NULL);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    incomingReduction = 3;
    avatar.currentHp = 1; // Player HP lives in PLAYER, not in the actor's enemy-HP field.
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(strcmp(eventOrder, "I") == 0 && lastIncoming.result.damageDealt == 7);
    assert(!lastIncoming.result.killed && PLAYER(0).currentHp == 993);
    assert(incomingAdjustments == 1 && hpChanges == 1 && source == NULL);
    assert(!lastIncoming.criticalKnown);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    incomingReduction = 10;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.result.hit && lastIncoming.result.damageDealt == 0);
    assert(incomingAdjustments == 1 && hpChanges == 0 && PLAYER(0).currentHp == 1000);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = nativeAbsorb = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_FIRE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(!lastIncoming.melee && lastIncoming.result.hit && lastIncoming.result.damageDealt == 0);
    assert(incomingAdjustments == 0 && hpChanges == 0);

    resetFixture();
    eventKitEnabled = TRUE;
    assert(!skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(strcmp(eventOrder, "I") == 0 && !lastIncoming.result.hit);
    assert(PLAYER(0).currentHp == 1000 && hpChanges == 0);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    PLAYER(0).currentHp = 5;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.result.killed && lastIncoming.result.damageDealt == 5);
    assert(strcmp(eventOrder, "I") == 0); // A player's death must not award outgoing kill credit.

    resetFixture();
    eventKitEnabled = nativeIncomingHit = healFromEvent = TRUE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.result.damageDealt == 10 && PLAYER(0).currentHp == 1000);
    assert(strcmp(eventOrder, "I") == 0 && hpChanges == 2);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = retaliateFromEvent = TRUE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(strcmp(eventOrder, "IHR") == 0 && lastIncoming.result.damageDealt == 10);
    assert(lastEvent.source.kind == HACKLIB_DAMAGE_SECONDARY && lastEvent.source.abilityRow == 9);
    assert(lastEvent.source.origin == &avatar && source == NULL);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = nativeRetaliate = TRUE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(strcmp(eventOrder, "HRI") == 0 && lastEvent.source.origin == &avatar);
    assert(lastIncoming.source.origin == &foe && currentMeleeResolution == NULL && source == NULL);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = nativeRetaliate = healOnOutgoingHit = TRUE;
    PLAYER(0).currentHp = 500;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(PLAYER(0).currentHp == 495 && lastIncoming.result.damageDealt == 10);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = healInsideHpChange = TRUE;
    PLAYER(0).currentHp = 500;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(PLAYER(0).currentHp == 495 && lastIncoming.result.damageDealt == 10);
    assert(hpChanges == 2 && currentHpObservation == NULL);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = nestedIncoming = TRUE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(PLAYER(0).currentHp == 980 && lastIncoming.result.damageDealt == 10);
    assert(strcmp(eventOrder, "II") == 0 && currentIncomingResolution == NULL && source == NULL);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    Actor secondPlayer = {0};
    secondPlayer.as.combat.playerIndex = 1;
    PLAYER(1).ownerActor = &secondPlayer;
    PLAYER(1).currentHp = 100;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &secondPlayer));
    assert(PLAYER(1).currentHp == 90 && PLAYER(0).currentHp == 1000);
    assert(lastIncoming.victim == &secondPlayer && lastIncoming.result.damageDealt == 10);

    resetFixture();
    eventKitEnabled = TRUE;
    PLAYER_STAT(0, STAT_FIREBURST) = PLAYER_STAT(0, STAT_VENOMOUS) = 1;
    assert(skillExpansion_resolvePveAttack(&avatar, &foe));
    assert(lastEvent.source.elementsKnown && lastEvent.source.elements == HACKLIB_ELEMENT_MASK);
    assert(!lastEvent.appliedStatusElementsKnown);

    resetFixture();
    eventKitEnabled = TRUE;
    PLAYER_STAT(0, STAT_FIREBURST) = 1;
    nativeHit = FALSE;
    assert(!skillExpansion_resolvePveAttack(&avatar, &foe));
    assert(!lastEvent.source.elementsKnown);

    resetFixture();
    eventKitEnabled = TRUE;
    PLAYER_STAT(0, STAT_FIREBURST) = 1;
    skillExpansion_dealDamage(&avatar, &foe, 20, HACKLIB_DAMAGE_ABILITY, 8);
    assert(!lastEvent.source.elementsKnown); // Weapon affixes do not describe arbitrary spells.

    resetFixture();
    eventKitEnabled = TRUE;
    skillExpansion_dealElementalDamage(&avatar, &foe, 20, HACKLIB_DAMAGE_ABILITY, 8,
                                       HACKLIB_ELEMENT_FIRE);
    assert(lastEvent.source.elementsKnown && lastEvent.source.elements == HACKLIB_ELEMENT_FIRE);
    assert(lastEvent.result.damageDealt == 20 && !lastEvent.appliedStatusElementsKnown);
    assert(source == NULL);
    eventCount = 0;
    skillExpansion_dealElementalDamage(&avatar, &foe, 20, HACKLIB_DAMAGE_ABILITY, 8,
                                       HACKLIB_ELEMENT_NONE);
    assert(lastEvent.source.elementsKnown && lastEvent.source.elements == HACKLIB_ELEMENT_NONE);
    s32 hpBefore = foe.currentHp;
    assert(
        !skillExpansion_dealElementalDamage(&avatar, &foe, 20, HACKLIB_DAMAGE_ABILITY, 8, 128).hit);
    assert(foe.currentHp == hpBefore);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_FIRE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.source.elementsKnown &&
           lastIncoming.source.elements == HACKLIB_ELEMENT_FIRE);
    assert(lastIncoming.appliedStatusElementsKnown && !lastIncoming.appliedStatusElements);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_POISON;
    avatar.as.combat.venomDuration = 90;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.source.elements == HACKLIB_ELEMENT_POISON);
    assert(lastIncoming.appliedStatusElements == HACKLIB_ELEMENT_POISON);
    assert(lastIncoming.result.damageDealt == 12); // Native poison contact also removes two HP.

    resetFixture();
    eventKitEnabled = nativeIncomingHit = nativeAbsorb = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_POISON;
    PLAYER(0).heroId = HERO_ID_GANDALF;
    Actor shield = {0};
    shield.as.shieldGandalf.shieldHealth = 10;
    PLAYER(0).shieldBubble = &shield;
    avatar.as.combat.venomDuration = 90;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.source.elements == HACKLIB_ELEMENT_POISON);
    assert(lastIncoming.appliedStatusElementsKnown && !lastIncoming.appliedStatusElements);
    assert(avatar.as.combat.venomDuration == 90 && lastIncoming.result.damageDealt == 0);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_POISON;
    PLAYER_STAT(0, STAT_POISON_RESIST_PERCENT) = 100;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.appliedStatusElementsKnown && !lastIncoming.appliedStatusElements);
    assert(avatar.as.combat.venomDuration == 0 && lastIncoming.result.damageDealt == 12);

    resetFixture();
    eventKitEnabled = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_POISON;
    assert(!skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.source.elementsKnown &&
           lastIncoming.source.elements == HACKLIB_ELEMENT_POISON);
    assert(lastIncoming.appliedStatusElementsKnown && !lastIncoming.appliedStatusElements);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = nativeRetaliate = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_FIRE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.source.elements == HACKLIB_ELEMENT_FIRE);
    assert(lastEvent.source.elementsKnown && !lastEvent.source.elements);

    resetFixture();
    eventKitEnabled = nativeIncomingHit = retaliateFromEvent = TRUE;
    foe.unitType = ENEMY_TYPE_ORC_ARCHER_FIRE;
    assert(skillExpansion_resolveEnemyAttackOnPlayer(&foe, &avatar));
    assert(lastIncoming.source.elements == HACKLIB_ELEMENT_FIRE);
    assert(!lastEvent.source.elementsKnown && source == NULL);
    for (u8 level = 1; level <= 5; level++)
    {
        resetFixture();
        *skillExpansion_activeLevel(0, 7) = level;
        skillExpansion_aragornTrackersMark(&avatar);
        assert(states[0].markFrames == MARK_BASE_FRAMES + MARK_FRAMES_PER_LEVEL * level);
        assert(bonusDamage(0, &foe, 100) == MARK_BASE_PERCENT + MARK_PERCENT_PER_LEVEL * level);
        generation++;
        assert(bonusDamage(0, &foe, 100) == 0);
        eliteTick(0, &avatar);
        assert(!states[0].markFrames);
        skillExpansion_aragornFlameOfTheWest(&avatar);
        assert(states[0].flameFrames == FLAME_BASE_FRAMES + FLAME_FRAMES_PER_LEVEL * level);
        states[0].flameFrames = 0;
        states[0].meter = METER_MAX;
        skillExpansion_aragornFlameOfTheWest(&avatar);
        assert(states[0].flameReforged &&
               states[0].flameFrames ==
                   FLAME_BASE_FRAMES + FLAME_FRAMES_PER_LEVEL * level + FLAME_FULL_FRAMES);
        skillExpansion_aragornRangersAmbush(&avatar);
        assert(states[0].ambushFrames == AMBUSH_BASE_FRAMES + AMBUSH_FRAMES_PER_LEVEL * level);
        states[0].ambushFrames = 1;
        eliteTick(0, &avatar);
        assert(!states[0].ambushFrames && !(avatar.spriteFlags & SPRITE_FLAG_STEALTH));
        for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            skillExpansion_players[0].activeSkills[row].spiritCost = row + 10;
        }
        skillExpansion_aragornReturnOfTheKing(&avatar);
        assert(states[0].returnFrames == RETURN_BASE_FRAMES + RETURN_FRAMES_PER_LEVEL * level);
        assert(*skillExpansion_activeLevel(0, 7) == level + RETURN_TEMPORARY_LEVELS);
        for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            assert(skillExpansion_players[0].activeSkills[row].spiritCost == 0);
        }
        states[0].returnFrames = 1;
        eliteTick(0, &avatar);
        assert(*skillExpansion_activeLevel(0, 7) == level);
        for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            assert(skillExpansion_players[0].activeSkills[row].spiritCost == row + 10);
        }
    }
    resetFixture();
    *skillExpansion_activeLevel(0, 7) = 1;
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 1;
    skillExpansion_aragornTrackersMark(&avatar);
    assert(states[0].markLevel == 2);
    for (u8 rank = 1; rank <= 5; rank++)
    {
        resetFixture();
        passiveLevels[SKILL_EXPANSION_PASSIVE_KINGS_PRESENCE] = rank;
        foe.actionState = ACTOR_STATE_STUNNED;
        assert(bonusDamage(0, &foe, 100) == PRESENCE_DAMAGE_PER_LEVEL * rank);
        resetFixture();
        passiveLevels[SKILL_EXPANSION_PASSIVE_BLADE_REFORGED] = rank;
        PLAYER(0).currentHp = 100;
        bladeReforged(0, &states[0], &avatar);
        assert(states[0].meter == METER_MAX &&
               states[0].bladeCooldown ==
                   BLADE_BASE_COOLDOWN_FRAMES - BLADE_COOLDOWN_FRAMES_PER_LEVEL * rank);
        resetFixture();
        PLAYER(0).currentHp = 100;
        handsOfTheKing(0, rank);
        assert(PLAYER(0).currentHp == 100 + 10 * HANDS_PERCENT_PER_LEVEL * rank);
    }
    puts("PASS: Aragorn levels 1..5, equipment bonuses, mark reuse, flame, ambush, Return expiry "
         "and passives");
    return 0;
}
