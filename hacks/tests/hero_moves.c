#define SKILL_EXPANSION_HERO_STATE_WORDS 40
#include "game.h"
#include "lib/skill_fx.h"
#include "lib/targeting.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "scene.h"
#include "stats.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Actor *testNextEnemy(Actor *owner, Actor *cursor);
#undef findNextEnemy
#define findNextEnemy testNextEnemy
#define asm(...)
#if TEST_SAM
#include "patches/skill_expansion/skill_expansion_sam.c"
#else
#include "patches/skill_expansion/skill_expansion_smeagol.c"
#endif
#undef asm

PlayerGlobals g_PlayerGlobals;
u16 g_PlayerKeysCurrent[2], g_PlayerKeysPressed[2], g_PlayerKeysReleased[2];
s32 rng_rollRange(s32 low, s32 high)
{
    (void)high;
    return low;
}
GameFlags g_GameFlags;
SceneState g_SceneCurrent;
ActorList g_ActorListMain;
SkillExpansionPlayer skillExpansion_players[2];
u32 skillExpansion_heroState[2][SKILL_EXPANSION_HERO_STATE_WORDS];
u8 skillExpansion_heroStateOwner[2];
u8 skillExpansion_heroStateMisuse;
void skillExpansion_noteHeroStateMisuse(void) { skillExpansion_heroStateMisuse++; }
static const u16 palette[16];
const u16 *ObjPalettes[90] = {[0 ... 89] = palette};
static Actor avatar, foe, visualActor;
static u32 foeGeneration = 1, visualGeneration = 1, hits, lastDamage, removals;
static u8 passives[SKILL_EXPANSION_PASSIVE_END];
static bool hasTarget, miss, safeLanding = TRUE, stunnable = TRUE;
static SkillExpansionBurst lastBurst;

void *memory_memset(void *dest, u32 value, size_t count)
{
    return memset(dest, value, count);
}
s32 skillExpansion_getPassiveLevel(u8 p, u8 id)
{
    (void)p;
    return passives[id];
}
Actor *hackLib_fxBurst(Actor *a, u8 shape, HackLibFxColor color, u8 layer)
{
    (void)a;
    (void)shape;
    (void)color;
    (void)layer;
    return &visualActor;
}
Actor *hackLib_fxPace(Actor *a, u8 ticks)
{
    (void)ticks;
    return a;
}
Actor *hackLib_fxAt(Actor *a, const Vector2Fp16 *position, u8 shape, HackLibFxColor color)
{
    (void)position;
    return hackLib_fxBurst(a, shape, color, 0);
}
Actor *hackLib_fxTimed(Actor *a, u8 shape, HackLibFxColor color, s32 frames, u8 layer)
{
    (void)frames;
    return hackLib_fxBurst(a, shape, color, layer);
}
bool32 hackLib_fxHold(Actor *a, u16 frames)
{
    (void)frames;
    return a != NULL;
}
void hackLib_fxRemove(Actor *a)
{
    assert(a == &visualActor);
    removals++;
}
void hackLib_fxTintSlot(u8 slot, const u16 *palette, u8 color, u8 blend, u16 contrast,
                        u8 brightness)
{
    (void)slot;
    (void)palette;
    (void)color;
    (void)blend;
    (void)contrast;
    (void)brightness;
}
u32 hackLib_playEffect(u16 id, Actor *a, const Vector2Fp16 *position, HackLibEffectHandle *handle)
{
    (void)id;
    (void)a;
    (void)position;
    (void)handle;
    return 0;
}
u32 sfx_play(s32 id)
{
    (void)id;
    return 0;
}
Actor *fx_attachStatusEffect(Actor *a, Actor *b, u16 frames, u8 kind, u8 arg4, u8 arg5, u8 flag)
{
    (void)a;
    (void)b;
    (void)frames;
    (void)kind;
    (void)arg4;
    (void)arg5;
    (void)flag;
    return NULL;
}
bool actor_isStunnable(Actor *a)
{
    (void)a;
    return stunnable;
}
void skillExpansion_feedback(u8 p, const char *text)
{
    (void)p;
    (void)text;
}
void player_addSpirit(u8 p, s16 amount)
{
    PLAYER(p).currentSpirit += amount;
    if (PLAYER(p).currentSpirit > PLAYER_STAT(p, STAT_MAX_SPIRIT))
    {
        PLAYER(p).currentSpirit = PLAYER_STAT(p, STAT_MAX_SPIRIT);
    }
}
void player_addHp(u8 p, s16 amount, bool flinch)
{
    (void)flinch;
    PLAYER(p).currentHp += amount;
    if (PLAYER(p).currentHp > PLAYER_STAT(p, STAT_MAX_HP))
    {
        PLAYER(p).currentHp = PLAYER_STAT(p, STAT_MAX_HP);
    }
}
static Actor *testNextEnemy(Actor *owner, Actor *cursor)
{
    (void)owner;
    return cursor == NULL && hasTarget ? &foe : NULL;
}
bool32 skillExpansion_isFoe(Actor *a)
{
    return a == &foe && foe.currentHp > 0 && !(foe.flags.p & ACTOR_FLAG_PENDING_REMOVE);
}
bool32 skillExpansion_isPlayerAvatar(Actor *a)
{
    return a == &avatar;
}
Actor *hackLib_nearestTarget(const HackLibTargetQuery *query)
{
    s32 dx = (foe.coords.x - query->center->coords.x) >> 16;
    return hasTarget && skillExpansion_isFoe(&foe) && ABS(dx) <= query->radius ? &foe : NULL;
}
Actor *hackLib_nextTarget(const HackLibTargetQuery *query, Actor *cursor)
{
    (void)query;
    return !cursor && hasTarget ? &foe : NULL;
}
HackLibDamageResult skillExpansion_dealDamage(Actor *a, Actor *b, u16 damage, u8 kind, u8 row)
{
    (void)a;
    (void)kind;
    (void)row;
    hits++;
    lastDamage = damage;
    s32 before = b->currentHp;
    if (!miss)
    {
        b->currentHp -= damage;
    }
    return hackLib_damageResult(before, b->currentHp, !miss);
}
u32 skillExpansion_burstDamage(const SkillExpansionBurst *burst)
{
    lastBurst = *burst;
    return hasTarget;
}
void skillExpansion_spawnBurstFx(Actor *a, u8 variant, s32 sound)
{
    (void)a;
    (void)variant;
    (void)sound;
}
bool hackLib_canLandAt(const Actor *a, const Vector2Fp16 *pos)
{
    (void)a;
    (void)pos;
    return safeLanding;
}
bool hackLib_findLanding(const Actor *a, const Vector2Fp16 *pos, Vector2Fp16 *out)
{
    (void)a;
    *out = *pos;
    return safeLanding;
}
HackLibActorRef hackLib_actorRef(Actor *a)
{
    return (HackLibActorRef){a, a == &foe ? foeGeneration : visualGeneration};
}
bool hackLib_isActorRefValid(HackLibActorRef ref)
{
    return ref.actor && ref.generation == hackLib_actorRef(ref.actor).generation &&
           !(ref.actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}
bool hackLib_isLiveActor(const Actor *a)
{
    return a == &foe || a == &avatar || a == &visualActor;
}
void loot_rollDrop(Actor *a)
{
    (void)a;
}
void skillExpansion_gandalfInput(u8 p)
{
    (void)p;
}
void skillExpansion_legolasInput(u8 p)
{
    (void)p;
}

void skillExpansion_original_input_update(void)
{
}

static void resetFixture(void)
{
    memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    memset(&avatar, 0, sizeof(avatar));
    memset(&foe, 0, sizeof(foe));
    memset(passives, 0, sizeof(passives));
    memset(skillExpansion_heroState, 0, sizeof(skillExpansion_heroState));
    PLAYER(0).heroId = TEST_SAM ? HERO_ID_SAM : HERO_ID_SMEAGOL;
    PLAYER(0).ownerActor = &avatar;
    PLAYER(0).currentHp = PLAYER_STAT(0, STAT_MAX_HP) = 1000;
    PLAYER(0).currentSpirit = PLAYER_STAT(0, STAT_MAX_SPIRIT) = 100;
    PLAYER(0).castingActiveSkill = 7;
    *skillExpansion_activeLevel(0, 7) = 1;
    avatar.facing = 2;
    avatar.coords = (Vector2Fp16){100 * 65536, 100 * 65536};
    foe.coords = (Vector2Fp16){150 * 65536, 100 * 65536};
    foe.currentHp = 1000;
    hasTarget = safeLanding = stunnable = TRUE;
    miss = FALSE;
    hits = lastDamage = removals = 0;
    foeGeneration = visualGeneration = 1;
    g_SceneCurrent.id = SCENE_ID_GAMEPLAY;
    g_GameFlags.p = 0;
}
int main(void)
{
#if TEST_SAM
    for (u8 level = 1; level <= 5; level++)
    {
        resetFixture();
        *skillExpansion_activeLevel(0, 7) = level;
        skillExpansion_samPlantTaters(&avatar);
        assert(state(0)->plants[0].level == level);
        avatar.coords.x = 60 * 65536;
        for (u32 i = 0; i < 180; i++)
        {
            skillExpansion_samTick(0, &avatar);
        }
        assert(state(0)->plants[0].growth == RIPE_GROWTH);
        skillExpansion_samSecondBreakfast(&avatar);
        assert(state(0)->stew.life == STEW_BASE_FRAMES + STEW_FRAMES_PER_LEVEL * level);
        PLAYER(0).currentHp = 500;
        skillExpansion_samTick(0, &avatar);
        assert(PLAYER(0).currentHp == 500 + 10 * (2 + level));
        skillExpansion_samFryingPanParry(&avatar);
        assert(state(0)->parryFrames == PARRY_BASE_FRAMES + PARRY_FRAMES_PER_LEVEL * level);
        assert(skillExpansion_samIncomingDamage(0, &foe, 100) == 0 && state(0)->parryCharges == 1);
        state(0)->parryFrames = 1;
        skillExpansion_samTick(0, &avatar);
        assert(lastBurst.damage == 6 + 3 * level);
    }
    resetFixture();
    skillExpansion_samPlantTaters(&avatar);
    passives[SKILL_EXPANSION_PASSIVE_GREEN_THUMB] = 1;
    avatar.coords.x = 60 * 65536;
    for (u32 i = 0; i < 150; i++)
    {
        skillExpansion_samTick(0, &avatar);
    }
    assert(isRipe(&state(0)->plants[0]));
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_MASTER_GARDENER] = 1;
    assert(gardenSlots(0) == 4);
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_STUBBORN] = 3;
    skillExpansion_samPlantTaters(&avatar);
    assert(skillExpansion_samIncomingDamage(0, &foe, 100) == 85);
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_I_CAN_CARRY_YOU] = 1;
    assert(skillExpansion_samIncomingDamage(0, &foe, 900) == 0 && state(0)->carried);
    resetFixture();
    hasTarget = FALSE;
    assert(skillExpansion_samElvenRope(&avatar) == SKILL_CAST_INVALID_TARGET);
    resetFixture();
    miss = TRUE;
    assert(skillExpansion_samElvenRope(&avatar) == SKILL_CAST_SUCCESS);
    assert(foe.actionState != ACTOR_STATE_STUNNED);
    resetFixture();
    skillExpansion_samPlantTaters(&avatar);
    state(0)->plants[0].growth = RIPE_GROWTH;
    miss = TRUE;
    assert(skillExpansion_samPoTayToes(&avatar) == SKILL_CAST_SUCCESS);
    assert(foe.actionState != ACTOR_STATE_STUNNED);
    resetFixture();
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 1;
    skillExpansion_samPlantTaters(&avatar);
    assert(state(0)->plants[0].level == 2);
    resetFixture();
    *skillExpansion_activeLevel(0, 7) = 5;
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 2;
    skillExpansion_samSecondBreakfast(&avatar);
    assert(state(0)->stew.level == 6);
    resetFixture();
    *skillExpansion_activeLevel(0, 7) = 0;
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 1;
    assert(skillExpansion_effectiveActiveLevel(0, 7) == 0);
    for (u8 rank = 1; rank <= 5; rank++)
    {
        resetFixture();
        passives[SKILL_EXPANSION_PASSIVE_SHIRE_GRIT] = rank;
        skillExpansion_samFryingPanParry(&avatar);
        skillExpansion_samIncomingDamage(0, &foe, 10);
        assert(skillExpansion_samDamage(0, 100, FALSE) == 100);
        assert(skillExpansion_samDamage(0, 100, TRUE) == 100 + 3 * rank);
        assert(skillExpansion_samDamage(0, 100, TRUE) == 100);
        resetFixture();
        passives[SKILL_EXPANSION_PASSIVE_FINE_SEASONING] = rank;
        PLAYER(0).currentSpirit = 0;
        SamPlant plant = {.level = 1};
        reap(0, &avatar, &plant, FALSE);
        assert(PLAYER(0).currentSpirit == 3 * rank);
        passives[SKILL_EXPANSION_PASSIVE_LEMBAS_CRUMBS] = rank;
        PLAYER(0).currentHp = 100;
        reap(0, &avatar, &plant, TRUE);
        assert(PLAYER(0).currentHp == 100 + 30 * rank);
    }
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_TATERS_FOR_ALL] = 1;
    skillExpansion_samSecondBreakfast(&avatar);
    skillExpansion_samTick(0, &avatar);
    assert(skillExpansion_samDamage(0, 100, FALSE) == 120);
    puts("PASS: Sam levels 1..5, garden growth, healing, parry and passive upgrades");
#else
    for (u8 level = 1; level <= 5; level++)
    {
        resetFixture();
        *skillExpansion_activeLevel(0, 7) = level;
        skillExpansion_smeagolPlayDead(&avatar);
        assert(state(0)->playDeadFrames ==
               PLAY_DEAD_BASE_FRAMES + PLAY_DEAD_FRAMES_PER_LEVEL * level);
        state(0)->getUp = TRUE;
        skillExpansion_smeagolTick(0, &avatar);
        assert(state(0)->strikeFrames && !(avatar.flags.p & ACTOR_FLAG_HIDDEN));
        assert(skillExpansion_smeagolDamage(0, 10, TRUE) == 30);
        skillExpansion_smeagolOnHit(0, &foe, (HackLibDamageResult){TRUE, FALSE, 30, 0}, TRUE);
        assert(!state(0)->strikeFrames);
        skillExpansion_smeagolCatchAFish(&avatar);
        assert(state(0)->catchesLeft == 1 + level / 2);
        state(0)->catchFrames = 0;
        foe.coords.x = avatar.coords.x + 20 * 65536;
        assert(skillExpansion_smeagolThrottle(&avatar) == SKILL_CAST_SUCCESS);
        assert(state(0)->throttleFrames ==
               THROTTLE_BASE_FRAMES + THROTTLE_FRAMES_PER_LEVEL * level);
        foeGeneration++;
        skillExpansion_smeagolTick(0, &avatar);
        assert(!state(0)->throttleFrames);
        assert(skillExpansion_smeagolSlink(&avatar) == SKILL_CAST_SUCCESS);
        assert(state(0)->hideFrames ==
               SLINK_BASE_HIDE_FRAMES + SLINK_HIDE_FRAMES_PER_LEVEL * level);
    }
    resetFixture();
    safeLanding = FALSE;
    assert(skillExpansion_smeagolSlink(&avatar) == SKILL_CAST_BLOCKED_LANDING);
    resetFixture();
    hasTarget = FALSE;
    assert(skillExpansion_smeagolThrottle(&avatar) == SKILL_CAST_INVALID_TARGET);
    resetFixture();
    state(0)->grip = 80;
    skillExpansion_smeagolArgueWithYourself(&avatar);
    assert(state(0)->grip == 50 && foe.actionState == ACTOR_STATE_STUNNED);
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_HUNGRY] = 2;
    state(0)->grip = 50;
    assert(skillExpansion_smeagolDamage(0, 100, FALSE) ==
           100 + 100 * HUNGRY_PERCENT_PER_LEVEL * 2 * 5 / 100);
    resetFixture();
    skillExpansion_smeagolDropThePrecious(&avatar);
    visualGeneration++;
    skillExpansion_smeagolCleanup(0, &avatar);
    assert(removals == 0);
    resetFixture();
    PLAYER_STAT(0, STAT_ACTIVE_SKILL_LEVEL_BONUS) = 1;
    skillExpansion_smeagolPlayDead(&avatar);
    assert(state(0)->playDeadFrames == PLAY_DEAD_BASE_FRAMES + 2 * PLAY_DEAD_FRAMES_PER_LEVEL);
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_NICE_MASTER] = 2;
    PLAYER(0).currentHp = 100;
    skillExpansion_smeagolPlayDead(&avatar);
    assert(PLAYER(0).currentHp == 100 + 1000 * NICE_MASTER_HEAL_PERCENT_PER_LEVEL * 2 / 100);
    resetFixture();
    passives[SKILL_EXPANSION_PASSIVE_WE_WANTS_IT] = 1;
    startTakeover(0, state(0), &avatar);
    assert(state(0)->takeoverFrames == TAKEOVER_FRAMES + WE_WANTS_IT_FRAMES);
    passives[SKILL_EXPANSION_PASSIVE_SMEAGOL_IS_FREE] = 1;
    state(0)->mashedFrames = TAKEOVER_FRAMES;
    PLAYER(0).currentSpirit = 0;
    endTakeover(0, state(0), &avatar);
    assert(state(0)->freeFrames == FREE_FRAMES && PLAYER(0).currentSpirit == 100);
    assert(skillExpansion_smeagolIncomingDamage(0, 100) == 0);
    puts("PASS: Smeagol levels 1..5, stealth, strike, catch, throttle, slink and grip upgrades");
#endif
    (void)hits;
    (void)lastDamage;
    (void)lastBurst;
    return 0;
}
