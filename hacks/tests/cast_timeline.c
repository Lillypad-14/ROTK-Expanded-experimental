#include "game.h"
#include "lib/skill_fx.h"
#include "patches/debug_cheats/ability_bench.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_timeline.h"
#include "scene.h"
#include "stats.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The host tests exercise the C cast path without the ARM HUD trampoline.
#define asm(...)
#include "patches/skill_expansion/skill_expansion_runtime.c"
#undef asm

static bool deliverySucceeds = TRUE;
static u32 deliveryLaunches;
static u8 deliveryLevel;
bool skillDelivery_launch(u8 p, const SkillDeliveryDef *definition, const Vector2Fp16 *position,
                          u8 direction, u8 level, u8 row, bool cosmetic)
{
    assert(p == 0 && definition->cooldownTicks == 60 && position == &PLAYER(p).ownerActor->coords);
    assert(direction == 8 && row == 7 && !cosmetic);
    deliveryLaunches++;
    deliveryLevel = level;
    return deliverySucceeds;
}
void skillDelivery_clear(u8 p)
{
    (void)p;
}
void skillDelivery_tick(u8 p, Actor *owner)
{
    (void)p;
    (void)owner;
}
void deliveryPreview_drawTelemetry(u8 p)
{
    (void)p;
}
ActorList g_ActorListMain;
PlayerHud g_PlayerHuds[2];
void debugLab_drawTelemetry(u8 p)
{
    (void)p;
}
void skillExpansion_original_player_tickHud(s32 p)
{
    (void)p;
}
void skillExpansion_frodoDrawBurdenBar(u8 p)
{
    (void)p;
}
void skillExpansion_aragornDrawFlameBar(u8 p)
{
    (void)p;
}
void skillExpansion_gimliDrawTallyBar(u8 p)
{
    (void)p;
}
void skillExpansion_gimliCastStarted(u8 p)
{
    (void)p;
}
void skillExpansion_gimliCastEnded(u8 p, bool success)
{
    (void)p;
    (void)success;
}
const char *skillExpansion_legolasRejectMessage(u8 p, u8 row, const char *message)
{
    (void)p;
    (void)row;
    return message;
}
void hackLib_clearOwnerEffects(Actor *actor)
{
    (void)actor;
}
void hackLib_clearOwnerSummons(Actor *actor)
{
    (void)actor;
}
void skillExpansion_eliteCleanup(u8 p, Actor *actor)
{
    (void)p;
    (void)actor;
}
static bool preservedFlame;
void skillExpansion_eliteReset(u8 p, bool preserveFlame)
{
    (void)p;
    preservedFlame = preserveFlame;
}
void skillExpansion_initHeroLoadout(u32 hero, u32 p)
{
    (void)hero;
    (void)p;
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
void debugLab_playerTick(u8 p)
{
    abilityBench_playerTick(p);
}
PlayerGlobals g_PlayerGlobals;
GameFlags g_GameFlags;
SceneState g_SceneCurrent;
SkillExpansionPlayer skillExpansion_players[2];
static Actor owner, target;
static u32 generation = 1, releases, visuals, allocations;
static bool failAllocation, reentrant;
static SkillExpansionCastResult result;
static const SkillExpansionCastPolicy policy = {1, 255, 255, 30};
static const HackLibDamageSource *damageSource;

void *memory_zalloc(size_t size)
{
    if (failAllocation)
    {
        return NULL;
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
void *memory_memset(void *dest, u32 value, size_t count)
{
    return memset(dest, value, count);
}
HackLibActorRef hackLib_actorRef(Actor *actor)
{
    return (HackLibActorRef){actor, generation};
}
bool hackLib_isActorRefValid(HackLibActorRef ref)
{
    return ref.actor && ref.generation == generation;
}
s32 hackLib_actorCurrentHp(const Actor *actor)
{
    return actor ? PLAYER(0).currentHp : 0;
}
u32 hackLib_actorGeneration(Actor *actor)
{
    (void)actor;
    return generation;
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
    return 1;
}
Actor *skillExpansion_findTarget(Actor *actor, u16 range, SkillExpansionTargetCondition condition,
                                 u32 arg)
{
    (void)actor;
    (void)range;
    (void)condition;
    (void)arg;
    return &target;
}
const SkillExpansionHeroKit *skillExpansion_getHeroKit(u32 hero)
{
    (void)hero;
    return NULL;
}
const SkillExpansionCastPolicy *skillExpansion_getCastPolicy(u8 p, u32 row)
{
    (void)p;
    (void)row;
    return &policy;
}
bool32 skillExpansion_activeCanCast(u8 p, u8 row)
{
    return !skillExpansion_runtime[p].pending && !skillExpansion_isTimelineBusy(p) &&
           *skillExpansion_activeLevel(p, row) > 0;
}
void player_addSpirit(u8 p, s16 amount)
{
    PLAYER(p).currentSpirit += amount;
}
const HackLibDamageSource *hackLib_swapDamageSource(const HackLibDamageSource *source)
{
    const HackLibDamageSource *previous = damageSource;
    damageSource = source;
    return previous;
}
static void release(Actor *actor)
{
    assert(actor == &owner);
    assert(damageSource && damageSource->kind == HACKLIB_DAMAGE_ABILITY);
    releases++;
    if (reentrant)
    {
        skillExpansion_clearTimeline(0);
    }
    skillExpansion_finishCast(0, result);
}
static void option(u8 index, u8 value)
{
    AbilityBenchOptions[index].set(value);
}
static void step(u32 ticks)
{
    while (ticks--)
    {
        skillExpansion_tickTimeline(0, &owner);
    }
}
static void reset(void)
{
    abilityBench_endSession();
    skillExpansion_clearTimeline(0);
    assert(allocations == 0);
    memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    memset(skillExpansion_players, 0, sizeof(skillExpansion_players));
    memset(skillExpansion_runtime, 0, sizeof(skillExpansion_runtime));
    memset(&owner, 0, sizeof(owner));
    PLAYER(0).ownerActor = &owner;
    PLAYER(0).activeSkills = skillExpansion_players[0].activeSkills;
    PLAYER(0).currentHp = 50;
    PLAYER(0).currentSpirit = PLAYER_STAT(0, STAT_MAX_SPIRIT) = 100;
    skillExpansion_players[0].activeSkills[5].callback = release;
    skillExpansion_players[0].activeSkills[5].spiritCost = 10;
    skillExpansion_runtime[0].callbacks[5] = release;
    *skillExpansion_activeLevel(0, 5) = 2;
    *skillExpansion_activePurchased(0, 5) = 2;
    g_SceneCurrent.id = SCENE_ID_GAMEPLAY;
    g_GameFlags.p = GAME_FLAG_BONUS_MAP_ACTIVE;
    releases = visuals = 0;
    result = SKILL_CAST_SUCCESS;
    reentrant = failAllocation = FALSE;
    for (u32 i = 0; i < ABILITY_BENCH_OPTION_COUNT; i++)
    {
        option(i, 0);
    }
    option(0, 5);
    option(1, 4);
    option(2, 1);
}
static void cast(void)
{
    option(4, 1);
    abilityBench_playerTick(0);
}
int main(void)
{
    reset();
    skillExpansion_cast(0, 5);
    assert(releases == 1 && !skillExpansion_runtime[0].pending);
    skillExpansion_completeCast(&owner);
    assert(releases == 1);
    assert(PLAYER(0).currentSpirit == 90);
    reset();
    PLAYER(0).heroId = HERO_ID_ARAGORN;
    g_ActorListMain.head = &owner;
    skillExpansion_runtimeBegin(0, HERO_ID_ARAGORN);
    assert(!preservedFlame);
    skillExpansion_runtimeBegin(0, HERO_ID_ARAGORN);
    assert(preservedFlame);
    PLAYER(0).currentHp = 0;
    skillExpansion_runtimeBegin(0, HERO_ID_ARAGORN);
    assert(!preservedFlame);
    PLAYER(0).currentHp = 50;
    generation++;
    skillExpansion_runtimeBegin(0, HERO_ID_ARAGORN);
    assert(!preservedFlame);
    skillExpansion_runtimeBegin(0, HERO_ID_GANDALF);
    assert(!preservedFlame);
    g_ActorListMain.head = NULL;
    puts("PASS: live stat refresh preserves Flame charge; death and hero replacement reset it");
    reset();
    cast();
    assert(*skillExpansion_activeLevel(0, 5) == 5);
    assert(*skillExpansion_activePurchased(0, 5) == 2);
    assert(PLAYER(0).currentSpirit == 90 && releases == 0 && visuals == 1);
    skillExpansion_completeCast(&owner);
    assert(releases == 0);
    step(5);
    assert(releases == 0);
    g_GameFlags.d.overlay = TRUE;
    step(20);
    assert(releases == 0);
    g_GameFlags.d.overlay = FALSE;
    step(1);
    assert(releases == 1 && visuals == 2 && !skillExpansion_runtime[0].pending);
    assert(skillExpansion_runtime[0].cooldowns[5] == 30 && skillExpansion_isTimelineBusy(0));
    skillExpansion_cast(0, 5);
    assert(releases == 1 && PLAYER(0).currentSpirit == 90);
    step(9);
    assert(!skillExpansion_isTimelineBusy(0));
    skillExpansion_completeCast(&owner);
    assert(releases == 1);
    abilityBench_playerTick(0);
    assert(*skillExpansion_activeLevel(0, 5) == 2 && allocations == 0);

    reset();
    cast();
    owner.actionState = ACTOR_STATE_HURT;
    step(1);
    assert(releases == 0 && PLAYER(0).currentSpirit == 100);
    assert(skillExpansion_runtime[0].lastResult == SKILL_CAST_INTERRUPTED);
    skillExpansion_completeCast(&owner);
    assert(releases == 0);
    abilityBench_playerTick(0);
    assert(allocations == 0);

    reset();
    result = SKILL_CAST_INVALID_TARGET;
    cast();
    step(6);
    assert(releases == 1 && PLAYER(0).currentSpirit == 100 && visuals == 1);
    assert(!skillExpansion_isTimelineBusy(0) && !skillExpansion_runtime[0].cooldowns[5]);

    reset();
    cast();
    option(5, 1);
    abilityBench_playerTick(0);
    assert(PLAYER(0).currentSpirit == 100 && !skillExpansion_isTimelineBusy(0));
    assert(*skillExpansion_activeLevel(0, 5) == 2);

    reset();
    reentrant = TRUE;
    cast();
    step(6);
    assert(releases == 1 && !skillExpansion_isTimelineBusy(0));
    abilityBench_playerTick(0);
    assert(allocations == 0);

    reset();
    failAllocation = TRUE;
    cast();
    assert(!skillExpansion_runtime[0].pending && PLAYER(0).currentSpirit == 100);
    failAllocation = FALSE;
    reset();
    option(3, 1);
    abilityBench_playerTick(0);
    PLAYER(0).currentSpirit = 1;
    skillExpansion_runtime[0].cooldowns[5] = 30;
    abilityBench_playerTick(0);
    assert(PLAYER(0).currentSpirit == 100 && !skillExpansion_runtime[0].cooldowns[5]);
    option(3, 0);
    abilityBench_playerTick(0);
    assert(*skillExpansion_activeLevel(0, 5) == 2);

    reset();
    g_GameFlags.p |= GAME_FLAG_COOP_ACTIVE;
    cast();
    assert(!releases && !allocations);
    reset();
    g_GameFlags.p = 0;
    cast();
    assert(!releases && !allocations);
    reset();
    option(2, 0);
    cast();
    assert(releases == 1 && !skillExpansion_isTimelineBusy(0));
    abilityBench_playerTick(0);
    assert(allocations == 0);

    reset();
    cast();
    generation++;
    step(1);
    assert(!skillExpansion_isTimelineBusy(0) && !releases && PLAYER(0).currentSpirit == 100);
    abilityBench_playerTick(0);
    assert(!allocations);
    reset();
    cast();
    step(6);
    owner.actionState = ACTOR_STATE_STUNNED;
    step(1);
    assert(releases == 1 && PLAYER(0).currentSpirit == 90 && !skillExpansion_isTimelineBusy(0));
    reset();
    option(3, 1);
    abilityBench_playerTick(0);
    failAllocation = TRUE;
    cast();
    assert(!skillExpansion_runtime[0].pending && PLAYER(0).currentSpirit == 100);
    failAllocation = FALSE;

    HackLibTimelineCursor cursor;
    HackLibCastTimeline invalid = {NULL, 3600, 1, 0};
    assert(!hackLib_startTimeline(&cursor, &invalid));
    invalid = (HackLibCastTimeline){NULL, 1, 1, 1};
    assert(!hackLib_startTimeline(&cursor, &invalid));
    reset();
    static const SkillDeliveryDef recipe = {.cooldownTicks = 60};
    static const SkillExpansionActiveDef definition = {.delivery = &recipe};
    skillExpansion_players[0].activeDefs[0] = &definition;
    skillExpansion_players[0].activeSkills[7].spiritCost = 10;
    *skillExpansion_activeLevel(0, 7) = 4;
    owner.facing = 2;
    skillExpansion_runtimeInstall(0);
    assert(PLAYER_ACTIVE_SKILL(0, 7).callback == skillExpansion_completeCast);
    skillExpansion_cast(0, 7);
    skillExpansion_completeCast(&owner);
    assert(deliveryLaunches == 1 && deliveryLevel == 4 && PLAYER(0).currentSpirit == 90);
    assert(skillExpansion_runtime[0].lastResult == SKILL_CAST_SUCCESS);
    assert(skillExpansion_runtime[0].cooldowns[7] == 60 && damageSource == NULL);
    skillExpansion_runtime[0].cooldowns[7] = 0;
    deliverySucceeds = FALSE;
    skillExpansion_cast(0, 7);
    skillExpansion_completeCast(&owner);
    assert(deliveryLaunches == 2 && PLAYER(0).currentSpirit == 90);
    assert(skillExpansion_runtime[0].lastResult == SKILL_CAST_UNAVAILABLE);
    assert(skillExpansion_runtime[0].cooldowns[7] == 0 && damageSource == NULL);
    reset();
    memset(g_PlayerHuds, 0, sizeof(g_PlayerHuds));
    skillExpansion_feedback(0, "Ability not ready");
    u32 redraws = 0;
    for (u32 frame = 0; frame < 90; frame++)
    {
        skillExpansion_prepareHud(0);
        assert(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_0);
        redraws += !!(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1);
        g_PlayerHuds[0].flags.p &= ~(PLAYERHUD_FLAG_0 | PLAYERHUD_FLAG_1 | HUD_TEXT_CLEARED);
        if (frame == 45)
        {
            skillExpansion_feedback(0, "Ability not ready");
        }
    }
    assert(redraws == 1);
    skillExpansion_feedback(0, "No valid target");
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1);
    g_PlayerHuds[0].flags.p = 0;
    char changingText[] = "Ability not ready";
    skillExpansion_feedback(0, changingText);
    skillExpansion_prepareHud(0);
    g_PlayerHuds[0].flags.p = 0;
    changingText[0] = 'a';
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1);
    g_PlayerHuds[0].flags.p = PLAYERHUD_FLAG_0;
    g_PlayerHuds[0].text = "Native pickup";
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].text != changingText && !skillExpansion_runtime[0].feedbackDrawn);
    g_PlayerHuds[0].flags.p = 0;
    skillExpansion_prepareHud(0);
    assert(strcmp(g_PlayerHuds[0].text, changingText) == 0 &&
           (g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1));
    g_PlayerHuds[0].flags.p = 0;
    g_GameFlags.p |= GAME_FLAG_OVERLAY;
    skillExpansion_prepareHud(0);
    assert(!skillExpansion_runtime[0].feedbackDrawn && !g_PlayerHuds[0].flags.p);
    g_GameFlags.p &= ~GAME_FLAG_OVERLAY;
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1);
    g_PlayerHuds[0].flags.p = HUD_TEXT_CLEARED;
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1);
    g_PlayerHuds[0].flags.p = 0;
    skillExpansion_runtime[0].feedbackFrames = 0;
    skillExpansion_prepareHud(0);
    assert(!g_PlayerHuds[0].flags.p && !skillExpansion_runtime[0].feedbackDrawn);
    skillExpansion_feedback(0, changingText);
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].flags.p & PLAYERHUD_FLAG_1);
    g_PlayerHuds[0].flags.p = 0;
    char longFeedback[120];
    memset(longFeedback, 'W', sizeof(longFeedback) - 1);
    longFeedback[sizeof(longFeedback) - 1] = 0;
    memset(g_PlayerHuds[0].field_0x6c, 0xa5, sizeof(g_PlayerHuds[0].field_0x6c));
    skillExpansion_feedback(0, longFeedback);
    skillExpansion_prepareHud(0);
    assert(g_PlayerHuds[0].textBuffer[sizeof(g_PlayerHuds[0].textBuffer) - 1] == 0);
    for (u32 i = 0; i < sizeof(g_PlayerHuds[0].field_0x6c); i++)
    {
        assert(g_PlayerHuds[0].field_0x6c[i] == 0xa5);
    }
    skillExpansion_prepareHud(255);
    puts("PASS: HUD feedback draws once, refreshes changed text, yields to native text and "
         "restores after overlays");
    puts("PASS: timeline release, recovery, pause, refund, cancellation, reentrancy and bench "
         "isolation");
    return 0;
}
