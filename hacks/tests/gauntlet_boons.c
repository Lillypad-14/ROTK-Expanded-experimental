#include "lib/actor_ref.h"
#include "lib/fx_kit.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "patches/gauntlet/gauntlet_boon_effects.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "variables.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Actor *testBoonSpawnNpc(u8 kind, Vector2Fp16 *position, Actor *owner, Actor *target,
                               u16 lifetime);
#undef spawnNpc
#define spawnNpc testBoonSpawnNpc
#define hackLib_fxAt(...) ((void)0)
#define hackLib_fxBurst(...) ((void)0)

#include "patches/gauntlet/gauntlet_boon_effects.c"
#include "patches/gauntlet/gauntlet_boons.c"

PlayerGlobals g_PlayerGlobals;
ActorList g_ActorListMain;
SkillExpansionRuntime skillExpansion_runtime[2];
SkillExpansionPlayer skillExpansion_players[2];
const void *HeroMeleeAnimationBanks[8];
static Actor actors[12];
static ActiveSkill activeSkills[SKILL_EXPANSION_ACTIVE_ROW_COUNT];
static u32 generations[12];
static bool running = TRUE;
static u32 randomValue, kills, projectiles, echoes, summons;
static const HackLibDamageSource *source;

bool game_isSceneChangePending(void)
{
    return FALSE;
}

bool32 gauntlet_isRunning(void)
{
    return running;
}
u32 gauntlet_roomsCleared(void)
{
    return 0;
}
void gauntlet_noteKill(void)
{
    kills++;
}
void gauntlet_addHerbs(u32 count)
{
    (void)count;
}
void gauntletLevel_refreshStats(u8 player)
{
    (void)player;
}
s32 rng_mod(u16 n)
{
    return randomValue % (n + 1);
}
u16 combat_getEnemyMaxHp(u16 type)
{
    (void)type;
    return 100;
}
Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 flagBit)
{
    return NULL;
}
bool actor_isStunnable(Actor *actor)
{
    return actor->currentHp > 0;
}
const SkillExpansionHeroKit *skillExpansion_getHeroKit(u32 hero)
{
    (void)hero;
    return NULL;
}
bool32 skillExpansion_isFoe(Actor *actor)
{
    return actor != NULL && actor->type == ACTOR_TYPE_ENEMY && actor->currentHp > 0 &&
           !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}
bool32 gauntlet_isLivingRunFoe(Actor *actor)
{
    return skillExpansion_isFoe(actor);
}
HackLibActorRef hackLib_actorRef(Actor *actor)
{
    for (u32 i = 0; i < 12; i++)
    {
        if (actor == &actors[i])
        {
            return (HackLibActorRef){actor, generations[i]};
        }
    }
    return (HackLibActorRef){0};
}
bool hackLib_isActorRefValid(HackLibActorRef reference)
{
    HackLibActorRef current = hackLib_actorRef(reference.actor);
    return reference.generation && current.generation == reference.generation &&
           !(reference.actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}
Actor *hackLib_nextTargetAt(const Vector2Fp16 *position, Actor *exclude, u16 radius, Actor *cursor)
{
    cursor = cursor != NULL ? cursor->next : g_ActorListMain.head;
    for (; cursor != NULL; cursor = cursor->next)
    {
        s32 dx = (cursor->coords.x - position->x) / 65536;
        s32 dy = (cursor->coords.y - position->y) / 65536;
        if (cursor != exclude && cursor->currentHp > 0 && dx * dx + dy * dy <= radius * radius)
        {
            return cursor;
        }
    }
    return NULL;
}
bool hackLib_canLandAt(const Actor *actor, const Vector2Fp16 *position)
{
    return TRUE;
}
void actor_lootPile_trigger(Actor *drop, Actor *owner)
{
    PLAYER(0).gems++;
    drop->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
}
const HackLibDamageSource *hackLib_currentDamageSource(void)
{
    return source;
}
void skillExpansion_feedback(u8 player, const char *message)
{
    (void)player;
    (void)message;
}
void skillExpansion_addHp(u8 player, s16 amount, bool flinch)
{
    PLAYER(player).currentHp += amount;
    if (PLAYER(player).currentHp > PLAYER_STAT(player, STAT_MAX_HP))
    {
        PLAYER(player).currentHp = PLAYER_STAT(player, STAT_MAX_HP);
    }
}
Actor *skillExpansion_launchProjectile(Actor *owner, u8 direction, u16 amount, u8 row)
{
    projectiles++;
    return NULL;
}
Actor *skillExpansion_summonGhost(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime,
                                  u16 amount, u8 cap, bool persistent)
{
    summons++;
    return NULL;
}
void skillExpansion_resolveTimelineCast(Actor *owner)
{
    echoes++;
    gauntletBoons_castResolved(0, PLAYER(0).castingActiveSkill, TRUE);
}
Direction8 math_vector2_direction8(Vector2Int vector)
{
    return vector.x > 0 ? DIRECTION8_EAST : DIRECTION8_WEST;
}
HackLibDamageResult skillExpansion_dealDamage(Actor *owner, Actor *target, u16 amount, u8 kind,
                                              u8 row)
{
    const HackLibDamageSource *previous = source;
    HackLibDamageSource nested = {owner, kind, row, previous != NULL ? previous->depth + 1 : 0,
                                  0,     TRUE};
    if (nested.depth >= HACKLIB_MAX_DAMAGE_DEPTH)
    {
        return (HackLibDamageResult){0};
    }
    source = &nested;
    s32 hpBefore = target->currentHp;
    target->currentHp -= gauntlet_damageDealt(0, target, amount);
    HackLibDamageResult result = hackLib_damageResult(hpBefore, target->currentHp, TRUE);
    HackLibCombatEvent event = {
        .attacker = owner, .victim = target, .source = nested, .result = result};
    gauntlet_combatEvent(0, &event);
    source = previous;
    return result;
}

void *memory_zalloc(size_t size)
{
    return calloc(1, size);
}
void memory_free(void *pointer)
{
    free(pointer);
}

static Actor *testBoonSpawnNpc(u8 kind, Vector2Fp16 *position, Actor *owner, Actor *target,
                               u16 lifetime)
{
    summons++;
    return NULL;
}
void actor_npc_tickPassive(Actor *actor)
{
    (void)actor;
}
void skillExpansion_trackGhost(Actor *actor)
{
    (void)actor;
}
const HackLibDamageSource *hackLib_swapDamageSource(const HackLibDamageSource *replacement)
{
    const HackLibDamageSource *previous = source;
    source = replacement;
    return previous;
}

static void reset(void)
{
    gauntletBoons_reset();
    memset(&g_PlayerGlobals, 0, sizeof(g_PlayerGlobals));
    memset(actors, 0, sizeof(actors));
    memset(skillExpansion_runtime, 0, sizeof(skillExpansion_runtime));
    memset(skillExpansion_players, 0, sizeof(skillExpansion_players));
    for (u32 i = 0; i < 12; i++)
    {
        generations[i]++;
    }
    running = TRUE;
    source = NULL;
    randomValue = kills = projectiles = echoes = summons = 0;
    PLAYER(0).ownerActor = &actors[0];
    PLAYER(0).activeSkills = activeSkills;
    PLAYER_STAT(0, STAT_MAX_HP) = PLAYER(0).currentHp = 200;
    PLAYER_STAT(0, STAT_MAX_SPIRIT) = PLAYER(0).currentSpirit = 100;
    for (u32 i = 1; i < 4; i++)
    {
        actors[i].type = ACTOR_TYPE_ENEMY;
        actors[i].currentHp = 100;
        actors[i].coords.x = i * 16 * 65536;
        actors[i].next = i < 3 ? &actors[i + 1] : NULL;
    }
    g_ActorListMain.head = &actors[1];
    gauntletBoons_reset();
}

static void hit(Actor *target, bool killed, s32 overkill)
{
    HackLibCombatEvent event = {
        .attacker = &actors[0],
        .victim = target,
        .source = {&actors[0], HACKLIB_DAMAGE_WEAPON, HACKLIB_ABILITY_NONE, 0, 0, TRUE},
        .result = {TRUE, killed, 10, overkill},
        .melee = TRUE};
    gauntlet_combatEvent(0, &event);
}

int main(void)
{
    reset();
    u8 fallbackOffers[GAUNTLET_OFFER_COUNT];
    assert(gauntletBoons_rollBestAvailable(fallbackOffers, RARITY_EPIC) == RARITY_EPIC);
    assert(fallbackOffers[0] != GAUNTLET_NO_BOON);
    for (u32 boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        if (Boons[boon].rarity >= RARITY_EPIC)
        {
            gauntletBoons_grant(boon);
        }
    }
    assert(gauntletBoons_rollBestAvailable(fallbackOffers, RARITY_EPIC) == RARITY_RARE);
    assert(fallbackOffers[0] != GAUNTLET_NO_BOON);
    for (u32 boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        if (Boons[boon].rarity >= RARITY_RARE)
        {
            gauntletBoons_grant(boon);
        }
    }
    assert(gauntletBoons_rollBestAvailable(fallbackOffers, RARITY_EPIC) == RARITY_COMMON);
    assert(fallbackOffers[0] != GAUNTLET_NO_BOON);
    for (u32 boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        gauntletBoons_grant(boon);
    }
    gauntletBoons_rollBestAvailable(fallbackOffers, RARITY_EPIC);
    assert(fallbackOffers[0] == GAUNTLET_NO_BOON);
    reset();
    for (u32 i = 0; i < GAUNTLET_BOON_COUNT; i++)
    {
        assert(gauntletBoons_name(i)[0] && gauntletBoons_description(i)[0]);
        gauntletBoons_grant(i);
        assert(gauntletBoons_stacks(i) == 1);
    }
    assert(gauntletBoons_ownedCount() == GAUNTLET_BOON_COUNT);
    s16 health = PLAYER(0).baseStats.health;
    gauntletBoons_grant(GAUNTLET_BOON_HEARTY);
    assert(PLAYER(0).baseStats.health == health);
    assert(!gauntletBoons_stacks(255));
    reset();
    assert(!eligible(GAUNTLET_BOON_STORM_PYRE));
    gauntletBoons_grant(GAUNTLET_BOON_CHAIN_LIGHTNING);
    gauntletBoons_grant(GAUNTLET_BOON_CORPSE_BLAST);
    assert(eligible(GAUNTLET_BOON_STORM_PYRE));
    assert(!eligible(GAUNTLET_BOON_ELVEN_VOLLEY));
    PLAYER(0).heroId = HERO_ID_LEGOLAS;
    assert(eligible(GAUNTLET_BOON_ELVEN_VOLLEY));
    for (u32 i = 0; i < GAUNTLET_BOON_COUNT; i++)
    {
        if (Boons[i].rarity != RARITY_LEGENDARY)
        {
            gauntletBoons_grant(i);
        }
    }
    u8 offers[3];
    gauntletBoons_roll(offers, RARITY_COMMON);
    u32 legendary = 0;
    for (u32 i = 0; i < 3; i++)
    {
        if (offers[i] != GAUNTLET_NO_BOON && Boons[offers[i]].rarity == RARITY_LEGENDARY)
        {
            legendary++;
        }
    }
    assert(legendary == 1);

    reset();
    gauntletBoons_grant(GAUNTLET_BOON_KINDLING);
    hit(&actors[1], FALSE, 0);
    for (u32 i = 0; i < 90; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(actors[1].currentHp == 91);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_BARROW_VENOM);
    for (u32 i = 0; i < 15; i++)
    {
        hit(&actors[1], FALSE, 0);
    }
    assert(findStatus(&actors[1], FALSE)->venomStacks == 10);
    for (u32 i = 0; i < 30; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(actors[1].currentHp == 90);
    generations[1]++;
    actors[1].currentHp = 100;
    for (u32 i = 0; i < 30; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(actors[1].currentHp == 100);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_CARADHRAS_CHILL);
    gauntletBoons_grant(GAUNTLET_BOON_SHATTER);
    for (u32 i = 0; i < 5; i++)
    {
        hit(&actors[1], FALSE, 0);
    }
    assert(actors[1].actionState == ACTOR_STATE_STUNNED);
    assert(findStatus(&actors[1], FALSE)->frozen == 60);
    assert(gauntlet_damageDealt(0, &actors[1], 10) == 20);

    reset();
    gauntletBoons_grant(GAUNTLET_BOON_BARROW_VENOM);
    gauntletBoons_grant(GAUNTLET_BOON_PLAGUE_SPREADER);
    gauntletBoons_grant(GAUNTLET_BOON_PANDEMIC);
    for (u32 i = 0; i < 8; i++)
    {
        hit(&actors[1], FALSE, 0);
    }
    actors[1].currentHp = 0;
    hit(&actors[1], TRUE, 0);
    assert(findStatus(&actors[2], FALSE)->venomStacks == 8);

    reset();
    gauntletBoons_grant(GAUNTLET_BOON_OVERKILL);
    actors[1].currentHp = -15;
    hit(&actors[1], TRUE, 15);
    gauntletBoonEffects_tick(0, &actors[0]);
    assert(actors[2].currentHp == 85);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_OVERKILL);
    gauntletBoons_grant(GAUNTLET_BOON_REAPERS_DUE);
    gauntletBoons_grant(GAUNTLET_BOON_CORPSE_BLAST);
    actors[1].currentHp = 0;
    hit(&actors[1], TRUE, 80);
    for (u32 i = 0; i < 3; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(actors[2].currentHp <= 0 && actors[3].currentHp > 0);
    assert(kills == 2);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_KEEN_EYE);
    gauntletBoons_grant(GAUNTLET_BOON_FLEETFOOT);
    for (u32 i = 0; i < 100; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(PLAYER_STAT(0, STAT_CRIT_CHANCE_PERCENT) == 10);
    assert(PLAYER_STAT(0, STAT_SPEED_PERCENT) == 12);
    PLAYER_STAT(0, STAT_SPEED_PERCENT) = 5;
    gauntletBoonEffects_statsRebuilt();
    gauntletBoonEffects_tick(0, &actors[0]);
    assert(PLAYER_STAT(0, STAT_SPEED_PERCENT) == 17);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_RITUAL);
    gauntletBoons_castStarted(0, 0, 100, 10);
    gauntletBoons_castStarted(0, 0, 90, 10);
    gauntletBoons_castStarted(0, 0, 80, 10);
    HackLibDamageSource cast = {&actors[0], HACKLIB_DAMAGE_ABILITY, 0, 0, 0, TRUE};
    source = &cast;
    assert(gauntlet_damageDealt(0, &actors[1], 10) == 30);
    source = NULL;
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_ECHO_CAST);
    gauntletBoons_grant(GAUNTLET_BOON_SPLIT_SHOT);
    gauntletBoons_grant(GAUNTLET_BOON_VOLLEY_ECHO);
    gauntletBoons_castResolved(0, 0, TRUE);
    assert(projectiles == 2);
    for (u32 i = 0; i < 15; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(echoes == 0 && projectiles == 6 && Extra.echoTicks == 0);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_MIRROR_IMAGE);
    gauntletBoons_castStarted(0, 0, 100, 10);
    gauntletBoons_castResolved(0, 0, TRUE);
    s32 amount = -20;
    bool flinch = TRUE;
    gauntletBoonEffects_hp(0, &amount, &flinch);
    assert(amount == 0);
    amount = -20;
    gauntletBoonEffects_hp(0, &amount, &flinch);
    assert(amount == -20);
    gauntletBoonEffects_roomStart();
    assert(effects == NULL);
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_RITUAL);
    gauntletBoons_grant(GAUNTLET_BOON_WRAITH_FORM);
    gauntletBoons_castStarted(0, 0, 100, 10);
    gauntletBoons_castResolved(0, 0, FALSE);
    assert(Extra.ritual == 0 && Extra.immunity == 0);
    running = FALSE;
    assert(!gauntletBoons_has(GAUNTLET_BOON_MIRROR_IMAGE));
    reset();
    PLAYER(0).heroId = HERO_ID_ARAGORN;
    gauntletBoons_grant(GAUNTLET_BOON_HEIR_OF_ISILDUR);
    actors[3].next = &actors[4];
    actors[4].type = ACTOR_TYPE_NPC;
    actors[4].variant = GHOST_NPC_KIND;
    actors[4].parentActor = &actors[0];
    ghostState(&actors[4])->lifetime = GHOST_LIFETIME_FRAMES;
    gauntletBoons_castResolved(0, ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD, TRUE);
    assert(ghostState(&actors[4])->lifetime == 2 * GHOST_LIFETIME_FRAMES);
    gauntletBoons_castResolved(0, ACTIVE_SKILL_ARAGORN_CALL_OF_THE_DEAD, TRUE);
    assert(ghostState(&actors[4])->lifetime == 2 * GHOST_LIFETIME_FRAMES);
    reset();
    PLAYER(0).heroId = HERO_ID_GANDALF;
    gauntletBoons_grant(GAUNTLET_BOON_YOU_SHALL_NOT_PASS);
    actors[3].next = &actors[4];
    actors[4].type = ACTOR_TYPE_PROJECTILE;
    actors[4].parentActor = &actors[1];
    gauntletBoons_castResolved(0, ACTIVE_SKILL_GANDALF_SHIELD, TRUE);
    for (u32 i = 0; i < 3; i++)
    {
        gauntletBoonEffects_tick(0, &actors[0]);
    }
    assert(actors[4].flags.p & ACTOR_FLAG_PENDING_REMOVE);
    assert(actors[1].currentHp == 90);
    for (u32 boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        reset();
        if (Boons[boon].heroPlusOne)
        {
            PLAYER(0).heroId = Boons[boon].heroPlusOne - 1;
        }
        if (Boons[boon].parentA)
        {
            gauntletBoons_grant(Boons[boon].parentA - 1);
        }
        if (Boons[boon].parentB)
        {
            gauntletBoons_grant(Boons[boon].parentB - 1);
        }
        gauntletBoons_grant(boon);
        *skillExpansion_activeLevel(0, 0) = 1;
        *skillExpansion_activeLevel(0, 1) = 2;
        gauntletBoons_roomStart();
        u8 row = boon == GAUNTLET_BOON_FAULT_LINE           ? ACTIVE_SKILL_GIMLI_EARTH_SHATTER
                 : boon == GAUNTLET_BOON_YOU_SHALL_NOT_PASS ? ACTIVE_SKILL_GANDALF_SHIELD
                                                            : 0;
        gauntletBoons_castStarted(0, row, 100, 10);
        gauntletBoons_castResolved(0, row, TRUE);
        HackLibDamageSource attack = {&actors[0], HACKLIB_DAMAGE_PROJECTILE, row, 0, 0, TRUE};
        source = &attack;
        skillExpansion_dealDamage(&actors[0], &actors[1], 10, HACKLIB_DAMAGE_PROJECTILE, row);
        source = NULL;
        for (u32 tick = 0; tick < 95; tick++)
        {
            gauntletBoons_tick(0, &actors[0], 1);
        }
        actors[2].currentHp = 0;
        hit(&actors[2], TRUE, 10);
        s32 heal = 20;
        bool flinch = TRUE;
        gauntletBoonEffects_hp(0, &heal, &flinch);
        gauntletBoonEffects_roomEnd();
        assert(effects == NULL && gauntletBoons_stacks(boon));
    }
    reset();
    gauntletBoons_grant(GAUNTLET_BOON_THE_ONE_RING);
    gauntletBoons_roomStart();
    gauntletBoonEffects_tick(0, &actors[0]);
    assert(PLAYER_STAT(0, STAT_MAX_HP) == 198);
    PLAYER_STAT(0, STAT_MAX_HP) = 210;
    gauntletBoonEffects_statsRebuilt();
    gauntletBoonEffects_tick(0, &actors[0]);
    assert(PLAYER_STAT(0, STAT_MAX_HP) == 208);
    gauntletBoonEffects_roomEnd();
    assert(PLAYER_STAT(0, STAT_MAX_HP) == 210);
    gauntletBoons_roomStart();
    gauntletBoonEffects_tick(0, &actors[0]);
    assert(PLAYER_STAT(0, STAT_MAX_HP) == 206);
    gauntletBoonEffects_roomEnd();
    printf("Gauntlet boon regressions passed (%u boons).\n", GAUNTLET_BOON_COUNT);
    return 0;
}
