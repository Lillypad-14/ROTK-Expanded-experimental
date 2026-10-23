// 64-bit host pointers widen the actor refs in SarumanState.
#define SKILL_EXPANSION_HERO_STATE_WORDS 32
#include "patches/ringwraith/saruman.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

PlayerGlobals g_PlayerGlobals;
SkillExpansionPlayer skillExpansion_players[2];
u32 skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS];
static Actor avatar, foe;
static bool hasTarget = TRUE;
static u32 generation = 1;
static u16 lastDamage;
static HackLibTimedEffect attachment;
static SkillExpansionBurst lastBurst;
static u8 passives[SKILL_EXPANSION_PASSIVE_END];
static HackLibDamageSource source = {.kind = HACKLIB_DAMAGE_ABILITY};
static const char *lastFeedback;

#define ANIMATION(name) const SpriteAnimation EnemyAnimationBank_EnemyTypeSaruman_##name[5] = {0}
ANIMATION(Stand);
ANIMATION(Walk);
ANIMATION(Stagger);
ANIMATION(Attack);
ANIMATION(Death);
#define ICON(name) const u8 ActiveSkillGandalf##name##Icon[0x184] = {0}
ICON(BlindingAura);
ICON(Lightstrike);
ICON(SwordOfPower);
ICON(Shield);
ICON(SummonGwaihir);
ICON(HerbalHealing);

Actor *hackLib_fxBurst(Actor *actor, u8 shape, HackLibFxColor color, u8 layer)
{
    (void)actor;
    (void)shape;
    (void)color;
    (void)layer;
    return NULL;
}
Actor *hackLib_fxTimed(Actor *on, u8 shape, HackLibFxColor color, s32 frames, u8 layer)
{
    (void)on;
    (void)shape;
    (void)color;
    (void)frames;
    (void)layer;
    return NULL;
}
Actor *hackLib_fxAt(Actor *origin, const Vector2Fp16 *pos, u8 shape, HackLibFxColor color)
{
    (void)origin;
    (void)pos;
    (void)shape;
    (void)color;
    return NULL;
}
Actor *hackLib_fxPace(Actor *fx, u8 frameTicks)
{
    (void)frameTicks;
    return fx;
}
void hackLib_fxDetach(Actor *fx, const Vector2Fp16 *pos)
{
    (void)fx;
    (void)pos;
}
u32 sfx_play(s32 sfxId)
{
    (void)sfxId;
    return 0;
}
Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 arg6)
{
    (void)parent;
    (void)target;
    (void)duration;
    (void)kind;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    return NULL;
}
bool actor_isStunnable(Actor *actor)
{
    (void)actor;
    return TRUE;
}
HackLibActorRef hackLib_actorRef(Actor *actor)
{
    return (HackLibActorRef){actor, generation};
}
bool hackLib_isActorRefValid(HackLibActorRef reference)
{
    return reference.actor != NULL && reference.generation == generation;
}
const HackLibDamageSource *hackLib_currentDamageSource(void)
{
    return &source;
}
void *memory_memset(void *dest, u32 value, size_t count)
{
    return memset(dest, value, count);
}
void player_addSpirit(u8 playerIndex, s16 delta)
{
    PLAYER(playerIndex).currentSpirit += delta;
}
void skillExpansion_feedback(u8 playerIndex, const char *message)
{
    (void)playerIndex;
    lastFeedback = message;
}
s32 skillExpansion_getPassiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    (void)playerIndex;
    return passives[passiveSkillId];
}
Actor *skillExpansion_findTarget(Actor *actor, u16 range, SkillExpansionTargetCondition filter,
                                 u32 argument)
{
    (void)actor;
    (void)range;
    (void)filter;
    (void)argument;
    return hasTarget ? &foe : NULL;
}
Actor *actor_iterateInRadius(Actor *actor, Actor *cursor, s32 radius)
{
    (void)actor;
    (void)radius;
    return cursor == NULL && hasTarget ? &foe : NULL;
}
bool32 skillExpansion_isFoe(Actor *actor)
{
    return actor == &foe;
}
u32 skillExpansion_burstDamage(const SkillExpansionBurst *burst)
{
    lastBurst = *burst;
    return hasTarget;
}
HackLibDamageResult skillExpansion_dealDamage(Actor *attacker, Actor *target, u16 damage, u8 kind,
                                              u8 row)
{
    (void)attacker;
    (void)target;
    lastDamage = damage;
    (void)kind;
    (void)row;
    return (HackLibDamageResult){TRUE, FALSE, 10};
}
void skillExpansion_addHp(u8 player, s16 amount, bool flinch)
{
    (void)flinch;
    PLAYER(player).currentHp += amount;
}
HackLibTimedEffect *hackLib_applyEffect(const HackLibEffectDef *definition, Actor *owner,
                                        Actor *target, u16 frames, s16 magnitude, u8 row)
{
    if (attachment.definition == definition && attachment.owner == owner &&
        attachment.target == target)
    {
        attachment.remaining = frames > attachment.remaining ? frames : attachment.remaining;
        return &attachment;
    }
    attachment = (HackLibTimedEffect){.definition = definition,
                                      .owner = owner,
                                      .target = target,
                                      .remaining = frames,
                                      .magnitude = magnitude,
                                      .abilityRow = row};
    return &attachment;
}
const HackLibTimedEffect *hackLib_effectAt(u32 index)
{
    return index == 0 && attachment.definition ? &attachment : NULL;
}
void hackLib_removeEffect(HackLibTimedEffect *effect)
{
    effect->definition = NULL;
}
const HackLibHudMeter HackLibHudGapMeter;
void hackLib_drawHudSegments(const HackLibHudMeter *meter, const HackLibHudMeterLook *const *looks,
                             u32 count, s32 value, s32 max)
{
    (void)meter;
    (void)looks;
    (void)count;
    (void)value;
    (void)max;
}
static EngineState engine;
EngineState *g_EngineState = &engine;
static SkillExpansionCastResult cast(u8 row, u8 level)
{
    PLAYER(0).castingActiveSkill = row;
    *skillExpansion_activeLevel(0, row) = level;
    return resolve(&avatar);
}
static u16 cost(u8 row)
{
    return skillExpansion_players[0].activeSkills[row].spiritCost;
}

Actor *hackLib_nextTargetAt(const Vector2Fp16 *position, Actor *exclude, u16 radius, Actor *cursor)
{
    (void)position;
    (void)exclude;
    return actor_iterateInRadius(NULL, cursor, (s32)radius * 65536);
}
int main(void)
{
    PLAYER(0).heroId = HERO_ID_SARUMAN;
    PLAYER(0).ownerActor = &avatar;
    PLAYER(0).level = 20;
    PLAYER_STAT(0, STAT_MAX_HP) = 100;
    PLAYER_STAT(0, STAT_MAX_SPIRIT) = 200;
    PLAYER(0).currentHp = 100;
    foe.currentHp = 50;
    reset(0);
    assert(state(0)->lastRow == ROW_NONE && cost(SARUMAN_VOICE) == 18);

    assert(cast(SARUMAN_VOICE, 2) == SKILL_CAST_SUCCESS);
    assert(lastBurst.damage == 10 && lastBurst.stunFrames == 75);
    assert(state(0)->prismMask == 1 << COLOR_VOICE && state(0)->lastRow == SARUMAN_VOICE);
    assert(cast(SARUMAN_VOICE, 2) == SKILL_CAST_SUCCESS && state(0)->prismMask == 1 << COLOR_VOICE);
    assert(cast(SARUMAN_ECHO, 1) == SKILL_CAST_SUCCESS && lastBurst.damage == 5);
    assert(state(0)->prismMask == ((1 << COLOR_VOICE) | (1 << COLOR_WHITE)));

    hasTarget = FALSE;
    assert(cast(SARUMAN_FLAME, 2) == SKILL_CAST_INVALID_TARGET);
    assert(cast(SARUMAN_BLASTING_FIRE, 2) == SKILL_CAST_INVALID_TARGET);
    hasTarget = TRUE;
    assert(cast(SARUMAN_FLAME, 2) == SKILL_CAST_SUCCESS);
    assert(lastFeedback == RefractionMessages[COLOR_ISENGARD] && state(0)->prismMask == 0);
    assert(state(0)->discount == DISCOUNT_HALF);
    tick(0, &avatar);
    assert(cost(SARUMAN_VOICE) == 9 && cost(SARUMAN_WHITE_HAND) == 45);
    assert(cast(SARUMAN_BLASTING_FIRE, 2) == SKILL_CAST_SUCCESS && lastBurst.damage == 44);
    tick(0, &avatar);
    assert(state(0)->discount == DISCOUNT_NONE && cost(SARUMAN_VOICE) == 18);

    passives[SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY] = 1;
    assert(cast(SARUMAN_FURNACE, 2) == SKILL_CAST_SUCCESS);
    assert(attachment.definition == &Burn && attachment.magnitude == 8 &&
           attachment.remaining == 240);

    assert(cast(SARUMAN_GUARD, 2) == SKILL_CAST_SUCCESS);
    assert(incomingDamage(0, &foe, 100) == 70);
    assert(cast(SARUMAN_HEAL, 2) == SKILL_CAST_UNAVAILABLE);
    PLAYER(0).currentHp = 20;
    assert(cast(SARUMAN_HEAL, 2) == SKILL_CAST_SUCCESS && PLAYER(0).currentHp == 48);

    PLAYER(0).currentHp = 100;
    assert(cast(SARUMAN_CURSE, 1) == SKILL_CAST_SUCCESS && state(0)->curseFrames == 540);
    assert(rangedDamage(0, &foe, 100) == 125);
    assert(cast(SARUMAN_BINDING, 2) == SKILL_CAST_SUCCESS && foe.actionStateTimer == 180);
    assert(rangedDamage(0, &foe, 100) == 150);
    source.kind = HACKLIB_DAMAGE_WEAPON;
    assert(rangedDamage(0, &foe, 100) == 100);
    assert(meleeDamage(0, &foe, 10) == 5);
    source.kind = HACKLIB_DAMAGE_ABILITY;

    PLAYER(0).currentSpirit = 0;
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, FALSE);
    assert(PLAYER(0).currentSpirit == CURSE_LEAK_SPIRIT);
    passives[SKILL_EXPANSION_PASSIVE_CUTPURSE] = 2;
    passives[SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR] = 1;
    passives[SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC] = 1;
    PLAYER(0).currentSpirit = 0;
    onKill(0, &avatar, &foe);
    assert(PLAYER(0).currentSpirit == 12 && state(0)->curseFrames != 0);
    assert(lastBurst.kind == HACKLIB_DAMAGE_SECONDARY && lastBurst.damage == 40);

    state(0)->prismMask = (1 << COLOR_VOICE) | (1 << COLOR_WHITE);
    PLAYER(0).currentSpirit = 0;
    assert(cast(SARUMAN_LIGHT_BROKEN, 1) == SKILL_CAST_SUCCESS && lastBurst.damage == 52);
    assert(PLAYER(0).currentSpirit == 16 && !canCastLightBroken(0));

    PLAYER(0).currentHp = 50;
    PLAYER(0).currentSpirit = 0;
    assert(cast(SARUMAN_SPITE, 1) == SKILL_CAST_SUCCESS && lastBurst.damage == 33);
    assert(PLAYER(0).currentSpirit == SPITE_STEAL_SPIRIT);

    passives[SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL] = 1;
    state(0)->lastColor = COLOR_WHITE;
    state(0)->prismMask = 1 << COLOR_WHITE;
    assert(cast(SARUMAN_WHITE_HAND, 255) == SKILL_CAST_SUCCESS);
    assert(lastFeedback == RefractionMessages[COLOR_WHITE] && state(0)->discount == DISCOUNT_FREE);
    tick(0, &avatar);
    assert(cost(SARUMAN_FURNACE) == 0);

    passives[SKILL_EXPANSION_PASSIVE_PARTING_SHOT] = 1;
    PLAYER(0).currentHp = 20;
    lastFeedback = NULL;
    tick(0, &avatar);
    assert(lastFeedback != NULL && state(0)->partingCooldown == PARTING_COOLDOWN_FRAMES);
    lastFeedback = NULL;
    tick(0, &avatar);
    assert(lastFeedback == NULL);

    PLAYER(0).currentHp = 0;
    tick(0, &avatar);
    assert(state(0)->discount == DISCOUNT_NONE && cost(SARUMAN_FURNACE) == 50);

    for (u8 level = 1; level <= 5; level++)
    {
        memory_memset(passives, 0, sizeof(passives));
        PLAYER(0).currentHp = 50;
        attachment.definition = NULL;
        reset(0);
        assert(cast(SARUMAN_FLAME, level) == SKILL_CAST_SUCCESS);
        assert(lastDamage == 10 + 7 * level && attachment.magnitude == 2 + level &&
               attachment.remaining == 150 + 30 * level);
        assert(cast(SARUMAN_GUARD, level) == SKILL_CAST_SUCCESS);
        assert(incomingDamage(0, &foe, 100) == 80 - 5 * level);
        PLAYER(0).currentHp = 1;
        assert(cast(SARUMAN_HEAL, level) == SKILL_CAST_SUCCESS &&
               PLAYER(0).currentHp == 13 + 8 * level);
        reset(0);
        passives[SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY] = level;
        attachment.definition = NULL;
        assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS && attachment.magnitude == 3 + level &&
               attachment.remaining == 180 + FIRES_FRAMES_PER_LEVEL * level);
    }
    memory_memset(passives, 0, sizeof(passives));
    reset(0);
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 1;
    assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS && lastDamage == 24);
    assert(cast(SARUMAN_FLAME, 5) == SKILL_CAST_SUCCESS && lastDamage == 52);
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 0;
    reset(0);
    assert(cast(SARUMAN_CURSE, 1) == SKILL_CAST_SUCCESS);
    passives[SKILL_EXPANSION_PASSIVE_CUTPURSE] = 2;
    generation++;
    PLAYER(0).currentSpirit = 0;
    assert(rangedDamage(0, &foe, 100) == 100);
    onKill(0, &avatar, &foe);
    assert(PLAYER(0).currentSpirit == 0);

    memory_memset(passives, 0, sizeof(passives));
    reset(0);
    attachment.definition = NULL;
    passives[SKILL_EXPANSION_PASSIVE_HONEYED_WORDS] = 2;
    assert(cast(SARUMAN_VOICE, 2) == SKILL_CAST_SUCCESS && lastBurst.stunFrames == 90);
    passives[SKILL_EXPANSION_PASSIVE_WISDOM_OF_THE_ISTARI] = 2;
    assert(cast(SARUMAN_ECHO, 1) == SKILL_CAST_SUCCESS && state(0)->prismFrames == 420);
    passives[SKILL_EXPANSION_PASSIVE_ROBES_OF_MANY_COLOURS] = 2;
    assert(incomingDamage(0, &foe, 100) == 92);

    passives[SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC] = 1;
    PLAYER(0).currentHp = 95;
    PLAYER(0).currentSpirit = 0;
    assert(cast(SARUMAN_HEAL, 1) == SKILL_CAST_SUCCESS && PLAYER(0).currentSpirit == 17);

    reset(0);
    attachment.definition = NULL;
    foe.actionState = 0;
    passives[SKILL_EXPANSION_PASSIVE_KINDLING] = 2;
    passives[SKILL_EXPANSION_PASSIVE_VOICE_OF_COMMAND] = 3;
    passives[SKILL_EXPANSION_PASSIVE_TREACHERY] = 2;
    passives[SKILL_EXPANSION_PASSIVE_STAFF_OF_ORTHANC] = 1;
    assert(rangedDamage(0, &foe, 100) == 100);
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, TRUE);
    assert(attachment.definition == &Burn && attachment.magnitude == 2 &&
           attachment.remaining == STAFF_BURN_FRAMES);
    assert(rangedDamage(0, &foe, 100) == 112);
    source.kind = HACKLIB_DAMAGE_PERIODIC;
    assert(rangedDamage(0, &foe, 100) == 100);
    source.kind = HACKLIB_DAMAGE_ABILITY;
    foe.actionState = ACTOR_STATE_STUNNED;
    assert(rangedDamage(0, &foe, 100) == 130);
    PLAYER(0).currentSpirit = 0;
    assert(meleeDamage(0, &foe, 10) == 5 && PLAYER(0).currentSpirit == 2);
    foe.actionState = 0;
    assert(cast(SARUMAN_FLAME, 2) == SKILL_CAST_SUCCESS && attachment.magnitude == 4);
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, TRUE);
    assert(attachment.magnitude == 4);

    passives[SKILL_EXPANSION_PASSIVE_WILDFIRE] = 2;
    onKill(0, &avatar, &foe);
    assert(state(0)->wildfirePending);
    attachment.definition = NULL;
    tick(0, &avatar);
    assert(!state(0)->wildfirePending && attachment.definition == &Burn &&
           attachment.remaining == WILDFIRE_FRAMES + 2 * WILDFIRE_FRAMES_PER_LEVEL);

    PLAYER(1).heroId = HERO_ID_FRODO;
    skillExpansion_heroState[1][0] = 0x1234;
    reset(1);
    tick(1, &avatar);
    assert(skillExpansion_heroState[1][0] == 0x1234);
    puts(
        "PASS: Saruman Prism, refractions, discounts, curse, binding, page 1 and Sharkey passives");
    return 0;
}
