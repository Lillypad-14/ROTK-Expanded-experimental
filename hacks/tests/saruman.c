// 64-bit host pointers widen the actor refs in SarumanState.
#define SKILL_EXPANSION_HERO_STATE_WORDS 32
#include "patches/ringwraith/saruman.c"
#include "patches/ringwraith/saruman_icons.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

PlayerGlobals g_PlayerGlobals;
SkillExpansionPlayer skillExpansion_players[2];
u32 skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS];
u8 skillExpansion_heroStateOwner[2];
u8 skillExpansion_heroStateMisuse;
void skillExpansion_noteHeroStateMisuse(void) { skillExpansion_heroStateMisuse++; }
static Actor avatar, foe;
static bool hasTarget = TRUE;
static u32 burstHits = 1;
static u32 generation = 1;
static u16 lastDamage;
static bool hit = TRUE, killed, recycleVictim;
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
#define ATTACK_ASSET(facing)                                                                       \
    const u8 EnemyAnimationTiles_EnemyTypeSaruman_Attack_Facing##facing[sizeof(                    \
        EnemyAnimationTiles_EnemyTypeSaruman_Attack_Facing##facing)] = {0};                        \
    const SpriteFrameSet EnemyAnimationFrames_EnemyTypeSaruman_Attack_Facing##facing = {0}
ATTACK_ASSET(0);
ATTACK_ASSET(1);
ATTACK_ASSET(2);
ATTACK_ASSET(3);
ATTACK_ASSET(4);
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
static u32 auraRequests;
Actor *hackLib_fxAura(Actor *on, u8 aura, HackLibFxColor color, s32 frames, u8 layer)
{
    assert(on != NULL && aura < 14 && frames > 0 && layer <= FX_LAYER_FRONT);
    (void)color;
    auraRequests++;
    return NULL;
}
bool32 fx_tickTimedAttachment(Actor *fx)
{
    (void)fx;
    return FALSE;
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
    if (PLAYER(playerIndex).currentSpirit > PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT))
    {
        PLAYER(playerIndex).currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    }
    if (PLAYER(playerIndex).currentSpirit < 0)
    {
        PLAYER(playerIndex).currentSpirit = 0;
    }
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
    return hasTarget ? burstHits : 0;
}
HackLibDamageResult skillExpansion_dealDamage(Actor *attacker, Actor *target, u16 damage, u8 kind,
                                              u8 row)
{
    (void)attacker;
    (void)target;
    lastDamage = damage;
    (void)kind;
    (void)row;
    if (recycleVictim)
    {
        generation++;
    }
    if (killed)
    {
        target->currentHp = 0;
    }
    return (HackLibDamageResult){hit, killed, hit ? 10 : 0};
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

static s32 hudValue, hudMax;
void hackLib_drawHudMeter(const HackLibHudMeter *meter, s32 value, s32 max,
                          const HackLibHudMeterLook *look, const u8 *marks, u32 markCount)
{
    (void)meter;
    (void)look;
    (void)marks;
    (void)markCount;
    hudValue = value;
    hudMax = max;
}

static void fixture(void)
{
    memset(passives, 0, sizeof(passives));
    memset(&attachment, 0, sizeof(attachment));
    PLAYER(0).heroId = HERO_ID_SARUMAN;
    PLAYER(0).ownerActor = &avatar;
    PLAYER(0).level = 20;
    PLAYER(0).currentHp = 100;
    PLAYER(0).currentSpirit = 0;
    PLAYER_STAT(0, STAT_MAX_HP) = 100;
    PLAYER_STAT(0, STAT_MAX_SPIRIT) = 200;
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 0;
    foe.currentHp = 100;
    foe.actionState = 0;
    hasTarget = TRUE;
    burstHits = 1;
    hit = TRUE;
    killed = recycleVictim = FALSE;
    auraRequests = 0;
    source.kind = HACKLIB_DAMAGE_ABILITY;
    lastFeedback = NULL;
    reset(0);
}

static void charge(void)
{
    for (u32 i = 0; i < EMPOWER_CAST_COUNT; i++)
    {
        assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS);
        assert(chargeCount(0) == i + 1);
        assert(lastDamage == 15);
    }
    assert(strstr(lastFeedback, "Empowered") != NULL);
}

int main(void)
{
    fixture();
    assert(cost(SARUMAN_VOICE) == 21 && state(0)->lastRow == ROW_NONE);
    for (u32 facing = 0; facing < 5; facing++)
    {
        assert(SarumanQuickAttackAnimations[facing].frameDuration == 1);
    }
    for (u32 pose = 0; pose < SARUMAN_PARTS * SARUMAN_POSES; pose++)
    {
        assert(SarumanMeleeAnimationBank[pose] == SarumanQuickAttackAnimations);
        assert(SarumanCastAnimationBank[pose] == SarumanQuickAttackAnimations);
    }
    charge();
    drawEmpowerment(0);
    assert(hudValue == 3 && hudMax == 3);
    for (u32 frame = 0; frame < 400; frame++)
    {
        tick(0, &avatar);
    }
    assert(chargeCount(0) == 3);
    assert(cast(SARUMAN_GUARD, 1) == SKILL_CAST_SUCCESS && chargeCount(0) == 3);
    PLAYER(0).currentHp = 1;
    assert(cast(SARUMAN_HEAL, 1) == SKILL_CAST_SUCCESS && PLAYER(0).currentHp == 21);
    assert(chargeCount(0) == 3);
    assert(cast(SARUMAN_ECHO, 1) == SKILL_CAST_SUCCESS && chargeCount(0) == 3);
    hasTarget = FALSE;
    assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_INVALID_TARGET && chargeCount(0) == 3);
    hasTarget = TRUE;
    assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS && lastDamage == 22);
    assert(chargeCount(0) == 0 && cost(SARUMAN_FLAME) == 21);
    assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS && lastDamage == 15 && chargeCount(0) == 1);

    // Every damaging row consumes readiness once, including Echo and periodic Furnace damage.
    const u8 damageRows[] = {
        SARUMAN_VOICE,   SARUMAN_FLAME, SARUMAN_BLASTING_FIRE, SARUMAN_BINDING, SARUMAN_ECHO,
        SARUMAN_FURNACE, SARUMAN_CURSE, SARUMAN_LIGHT_BROKEN,  SARUMAN_SPITE,   SARUMAN_WHITE_HAND};
    for (u32 i = 0; i < ARRAY_COUNT(damageRows); i++)
    {
        fixture();
        charge();
        hasTarget = FALSE;
        assert(cast(damageRows[i], 1) == SKILL_CAST_INVALID_TARGET && chargeCount(0) == 3);
        hasTarget = TRUE;
        assert(cast(damageRows[i], 1) == SKILL_CAST_SUCCESS && chargeCount(0) == 0);
        if (damageRows[i] == SARUMAN_ECHO)
        {
            assert(lastDamage == 11);
        }
        if (damageRows[i] == SARUMAN_FURNACE)
        {
            assert(attachment.magnitude == 6);
        }
        if (damageRows[i] == SARUMAN_BLASTING_FIRE)
        {
            assert(lastBurst.damage == 40);
        }
    }
    fixture();
    assert(cast(SARUMAN_LIGHT_BROKEN, 1) == SKILL_CAST_SUCCESS && lastBurst.damage == 38);
    assert(chargeCount(0) == 1);
    fixture();
    assert(cast(SARUMAN_WHITE_HAND, 1) == SKILL_CAST_SUCCESS && lastBurst.damage == 48);
    assert(chargeCount(0) == 1);
    fixture();
    hasTarget = FALSE;
    assert(cast(SARUMAN_VOICE, 1) == SKILL_CAST_INVALID_TARGET && chargeCount(0) == 0);
    assert(cast(SARUMAN_ECHO, 1) == SKILL_CAST_UNAVAILABLE && chargeCount(0) == 0);
    assert(cast(SARUMAN_HEAL, 1) == SKILL_CAST_UNAVAILABLE && chargeCount(0) == 0);

    for (u8 level = 1; level <= 5; level++)
    {
        fixture();
        assert(cast(SARUMAN_FLAME, level) == SKILL_CAST_SUCCESS);
        assert(lastDamage == 10 + 5 * level && attachment.magnitude == 1 + level &&
               attachment.remaining == 120 + 30 * level);
        assert(state(0)->bolt.trail == FXID_FLAME_TRAIL);
        assert(cast(SARUMAN_GUARD, level) == SKILL_CAST_SUCCESS);
        assert(incomingDamage(0, &foe, 100) == 80 - 5 * level);
        fixture();
        PLAYER(0).currentHp = 1;
        assert(cast(SARUMAN_HEAL, level) == SKILL_CAST_SUCCESS &&
               PLAYER(0).currentHp == 13 + 8 * level);
        fixture();
        passives[SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY] = level;
        assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS && attachment.magnitude == 2 + level);
        fixture();
        charge();
        passives[SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS] = level;
        passives[SKILL_EXPANSION_PASSIVE_RING_LORE] = level;
        passives[SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL] = 1;
        assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS);
        assert(lastDamage == 15 * (150 + 15 * level) / 100);
        assert(PLAYER(0).currentSpirit == 2 * level + COUNCIL_SPIRIT);
        assert(cost(SARUMAN_FLAME) == 21);
    }
    fixture();
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 1;
    assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS && lastDamage == 20);
    fixture();
    assert(cast(SARUMAN_CURSE, 1) == SKILL_CAST_SUCCESS && state(0)->curseFrames == 540);
    assert(rangedDamage(0, &foe, 100) == 125);
    assert(cast(SARUMAN_BINDING, 2) == SKILL_CAST_SUCCESS && foe.actionStateTimer == 180);
    assert(rangedDamage(0, &foe, 100) == 150);
    generation++;
    assert(rangedDamage(0, &foe, 100) == 100);
    passives[SKILL_EXPANSION_PASSIVE_CUTPURSE] = 2;
    onKill(0, &avatar, &foe);
    assert(PLAYER(0).currentSpirit == 0);
    fixture();
    charge();
    passives[SKILL_EXPANSION_PASSIVE_ROBES_OF_MANY_COLOURS] = 2;
    assert(incomingDamage(0, &foe, 100) == 88);
    fixture();
    passives[SKILL_EXPANSION_PASSIVE_WISDOM_OF_THE_ISTARI] = 2;
    passives[SKILL_EXPANSION_PASSIVE_TREACHERY] = 2;
    foe.actionState = ACTOR_STATE_STUNNED;
    assert(meleeDamage(0, &foe, 10) == 10 && !PLAYER(0).currentSpirit);
    onHit(0, &avatar, &foe, (HackLibDamageResult){FALSE, FALSE, 0}, TRUE);
    assert(!PLAYER(0).currentSpirit);
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, TRUE);
    assert(PLAYER(0).currentSpirit == STAFF_HIT_SPIRIT + 4);
    assert(chargeCount(0) == 0);
    fixture();
    charge();
    state(0)->curseFrames = state(0)->boundFrames = 500;
    state(0)->wildfirePending = TRUE;
    PLAYER(0).currentHp = 0;
    tick(0, &avatar);
    assert(!chargeCount(0) && !state(0)->curseFrames && !state(0)->boundFrames &&
           !state(0)->wildfirePending);
    fixture();
    passives[SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC] = 1;
    PLAYER(0).currentHp = 95;
    assert(cast(SARUMAN_HEAL, 1) == SKILL_CAST_SUCCESS &&
           PLAYER(0).currentSpirit == REMEDY_SPIRIT_CAP);
    fixture();
    passives[SKILL_EXPANSION_PASSIVE_STAFF_OF_ORTHANC] = 1;
    passives[SKILL_EXPANSION_PASSIVE_KINDLING] = 2;
    passives[SKILL_EXPANSION_PASSIVE_VOICE_OF_COMMAND] = 3;
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, TRUE);
    assert(attachment.definition == &Burn && attachment.magnitude == 2);
    assert(rangedDamage(0, &foe, 100) == 112);
    source.kind = HACKLIB_DAMAGE_PERIODIC;
    assert(rangedDamage(0, &foe, 100) == 100);
    source.kind = HACKLIB_DAMAGE_ABILITY;
    foe.actionState = ACTOR_STATE_STUNNED;
    assert(rangedDamage(0, &foe, 100) == 130);
    passives[SKILL_EXPANSION_PASSIVE_WILDFIRE] = 2;
    onKill(0, &avatar, &foe);
    assert(state(0)->wildfirePending);
    attachment.definition = NULL;
    tick(0, &avatar);
    assert(!state(0)->wildfirePending && attachment.definition == &Burn);
    assert(!chargeCount(0));
    fixture();
    assert(cast(SARUMAN_CURSE, 1) == SKILL_CAST_SUCCESS);
    passives[SKILL_EXPANSION_PASSIVE_CUTPURSE] = 2;
    passives[SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR] = 1;
    passives[SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC] = 1;
    onKill(0, &avatar, &foe);
    assert(PLAYER(0).currentSpirit == 7 && lastBurst.kind == HACKLIB_DAMAGE_SECONDARY);
    assert(lastBurst.damage == 40);
    fixture();
    passives[SKILL_EXPANSION_PASSIVE_PARTING_SHOT] = 1;
    PLAYER(0).currentHp = 20;
    tick(0, &avatar);
    assert(state(0)->partingCooldown == PARTING_COOLDOWN_FRAMES && !chargeCount(0));
    fixture();
    for (u32 frame = 0; frame < REGEN_FRAMES; frame++)
    {
        tick(0, &avatar);
    }
    assert(PLAYER(0).currentSpirit == 1);
    PLAYER(0).currentSpirit = PLAYER_STAT(0, STAT_MAX_SPIRIT);
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, TRUE);
    assert(PLAYER(0).currentSpirit == PLAYER_STAT(0, STAT_MAX_SPIRIT));
    for (u32 failure = 0; failure < 3; failure++)
    {
        fixture();
        hit = failure != 0;
        killed = failure == 1;
        recycleVictim = failure == 2;
        assert(cast(SARUMAN_BINDING, 1) == SKILL_CAST_SUCCESS);
        assert(!state(0)->boundFrames && !auraRequests);
        fixture();
        hit = failure != 0;
        killed = failure == 1;
        recycleVictim = failure == 2;
        assert(cast(SARUMAN_FLAME, 1) == SKILL_CAST_SUCCESS);
        assert(attachment.definition == NULL);
    }
    fixture();
    assert(cast(SARUMAN_BINDING, 1) == SKILL_CAST_SUCCESS && auraRequests == 1);
    PLAYER(1).heroId = HERO_ID_FRODO;
    skillExpansion_heroState[1][0] = 0x1234;
    reset(1);
    assert(skillExpansion_heroState[1][0] == 0x1234);
    // Runtime costs must match the menu rows for base and advanced spells.
    const u8 costRows[] = {SARUMAN_VOICE,        SARUMAN_FLAME,   SARUMAN_BLASTING_FIRE,
                           SARUMAN_GUARD,        SARUMAN_BINDING, SARUMAN_HEAL,
                           SARUMAN_ECHO,         SARUMAN_FURNACE, SARUMAN_CURSE,
                           SARUMAN_LIGHT_BROKEN, SARUMAN_SPITE,   SARUMAN_WHITE_HAND};
    const u16 expectedCosts[] = {21, 21, 42, 35, 28, 37, 37, 63, 35, 63, 52, 104};
    fixture();
    for (u32 i = 0; i < ARRAY_COUNT(costRows); i++)
    {
        assert(cost(costRows[i]) == expectedCosts[i]);
    }
    for (u8 rank = 1; rank <= 5; rank++)
    {
        fixture();
        assert(cast(SARUMAN_BLASTING_FIRE, rank) == SKILL_CAST_SUCCESS);
        assert(lastBurst.damage == 20 + 7 * rank);
        fixture();
        assert(cast(SARUMAN_LIGHT_BROKEN, rank) == SKILL_CAST_SUCCESS);
        assert(lastBurst.damage == 28 + 10 * rank);
        fixture();
        assert(cast(SARUMAN_WHITE_HAND, rank) == SKILL_CAST_SUCCESS);
        assert(lastBurst.damage == 36 + 12 * rank);
        fixture();
        assert(cast(SARUMAN_FURNACE, rank) == SKILL_CAST_SUCCESS);
        assert(attachment.magnitude == 2 + 2 * rank && attachment.remaining == 180);
        fixture();
        assert(cast(SARUMAN_FLAME, 5) == SKILL_CAST_SUCCESS);
        assert(cast(SARUMAN_ECHO, rank) == SKILL_CAST_SUCCESS);
        assert(lastDamage == 35 * (40 + 10 * rank) / 100);

        // Even a nearly full heal with maximum Keeper cannot pay for itself or its Echo.
        fixture();
        passives[SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC] = 5;
        PLAYER(0).currentHp = 99;
        PLAYER(0).currentSpirit = 100;
        assert(cast(SARUMAN_HEAL, rank) == SKILL_CAST_SUCCESS);
        player_addSpirit(0, -(s32)cost(SARUMAN_HEAL));
        assert(PLAYER(0).currentSpirit == 71);
        PLAYER(0).currentHp = 99;
        assert(cast(SARUMAN_ECHO, rank) == SKILL_CAST_SUCCESS);
        player_addSpirit(0, -(s32)cost(SARUMAN_ECHO));
        assert(PLAYER(0).currentSpirit == 42);
    }
    // Maximum empowerment refunds still leave a four-Flame cycle costing 68 Spirit.
    fixture();
    passives[SKILL_EXPANSION_PASSIVE_RING_LORE] = 5;
    passives[SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL] = 1;
    PLAYER(0).currentSpirit = 200;
    for (u32 i = 0; i < 4; i++)
    {
        player_addSpirit(0, -(s32)cost(SARUMAN_FLAME));
        assert(cast(SARUMAN_FLAME, 5) == SKILL_CAST_SUCCESS);
    }
    assert(PLAYER(0).currentSpirit == 132 && chargeCount(0) == 0);
    fixture();
    burstHits = 30;
    assert(cast(SARUMAN_SPITE, 5) == SKILL_CAST_SUCCESS);
    assert(PLAYER(0).currentSpirit == SPITE_SPIRIT_CAP);
    fixture();
    assert(cast(SARUMAN_CURSE, 1) == SKILL_CAST_SUCCESS);
    source.kind = HACKLIB_DAMAGE_PERIODIC;
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, FALSE);
    assert(PLAYER(0).currentSpirit == 0);
    source.kind = HACKLIB_DAMAGE_ABILITY;
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 0}, FALSE);
    assert(PLAYER(0).currentSpirit == 0);
    onHit(0, &avatar, &foe, (HackLibDamageResult){TRUE, FALSE, 10}, FALSE);
    assert(PLAYER(0).currentSpirit == CURSE_LEAK_SPIRIT);
    puts("PASS: Saruman three-cast empowerment, damage rows, support/Echo preservation, failures, "
         "upgrades and cleanup");
    return 0;
}
