#include "patches/gauntlet/gauntlet_boons.h"

#include "enemy.h"
#include "lib/fx_kit.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "player.h"
#include "rng.h"
#include "stats.h"
#include "variables.h"

typedef struct GauntletBoon
{
    const char *name;
    // Shown on the pick screen, wrapped to two or three lines.
    const char *description;
    u8 rarity;
} GauntletBoon;

static const GauntletBoon Boons[GAUNTLET_BOON_COUNT] = {
    [GAUNTLET_BOON_EDGE_OF_ANDURIL] = {"Edge of Anduril", "All your damage is increased by 15%.",
                                       RARITY_COMMON},
    [GAUNTLET_BOON_DEEP_BREATH] = {"Deep Breath", "Each second: regain 3% max Spirit +1.",
                                   RARITY_COMMON},
    [GAUNTLET_BOON_HOBBIT_LUCK] = {"Hobbit Luck",
                                   "Each enemy hit has a 10% chance to deal no damage.",
                                   RARITY_COMMON},
    [GAUNTLET_BOON_CHAIN_LIGHTNING] =
        {"Chain Lightning",
         "Your hits have a 15% chance to arc to 2 nearby foes for half "
         "damage.",
         RARITY_RARE},
    [GAUNTLET_BOON_BLOODTHIRST] = {"Bloodthirst", "Each kill: heal 2% max HP +1.", RARITY_RARE},
    [GAUNTLET_BOON_EXECUTIONER] = {"Executioner", "Triple damage to foes below 25% of their HP.",
                                   RARITY_RARE},
    [GAUNTLET_BOON_CORPSE_BLAST] =
        {"Corpse Blast", "Slain foes explode, dealing half their max HP to foes around them.",
         RARITY_RARE},
    [GAUNTLET_BOON_SECOND_WIND] = {"Second Wind",
                                   "Once per room, below 20% HP: heal 40% and stun nearby foes.",
                                   RARITY_RARE},
    [GAUNTLET_BOON_BLOOD_PRICE] = {"Blood Price", "Deal 25% more damage while at full HP.",
                                   RARITY_RARE},
    [GAUNTLET_BOON_HASTE_LOOP] = {"Haste Loop",
                                  "Every kill cuts 1 second off your skill cooldowns.",
                                  RARITY_EPIC},
    [GAUNTLET_BOON_LAST_STAND] = {"Last Stand",
                                  "Once per room, a killing blow leaves you at 1 HP, invulnerable "
                                  "with triple damage for 5s.",
                                  RARITY_LEGENDARY},
    [GAUNTLET_BOON_EYE_OF_SAURON] = {"Eye of Sauron",
                                     "Every 10s: hit the healthiest foe for 25% of its max HP +10.",
                                     RARITY_LEGENDARY},
};

static const char *const RarityNames[] = {"Common", "Rare", "Epic", "Legendary"};

// Offer weights by rarity.
static const u8 RarityWeights[] = {8, 5, 3, 1};

#define TICKS_PER_SECOND 30
#define CHAIN_RADIUS 64
#define CORPSE_RADIUS 48
#define SECOND_WIND_RADIUS 64
#define LAST_STAND_TICKS (5 * TICKS_PER_SECOND)
#define EYE_INTERVAL (10 * TICKS_PER_SECOND)
#define HASTE_LOOP_FRAMES 30

struct GauntletBoonState
{
    // bit per enum GauntletBoonId
    u16 owned;
    // ticks left of an active Last Stand
    u16 lastStandTicks;
    u16 eyeTicks;
    u8 spiritTicks;
    // bit 0: Second Wind spent this room, bit 1: Last Stand spent this room
    u8 spent;
    // TRUE while a boon deals its own damage, so it cannot trigger boons again
    u8 busy;
};

#define SPENT_SECOND_WIND 1
#define SPENT_LAST_STAND 2

static struct GauntletBoonState Boon;

static bool32 owns(u8 boon)
{
    return (Boon.owned >> boon) & 1;
}

u32 gauntletBoons_stacks(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? owns(boon) : 0;
}

void gauntletBoons_reset(void)
{
    Boon = (struct GauntletBoonState){0};
}

void gauntletBoons_roomStart(void)
{
    Boon.spent = 0;
}

void gauntletBoons_grant(u8 boon)
{
    if (boon < GAUNTLET_BOON_COUNT)
    {
        Boon.owned |= 1 << boon;
    }
}

const char *gauntletBoons_name(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? Boons[boon].name : "";
}

const char *gauntletBoons_description(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? Boons[boon].description : "";
}

void gauntletBoons_grantRandom(u8 rarity)
{
    u32 count = 0;
    u32 pick;
    u32 boon;

    for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        count += Boons[boon].rarity == rarity && !owns(boon);
    }
    if (count == 0)
    {
        return;
    }
    pick = gauntlet_randBelow(count);
    for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        if (Boons[boon].rarity == rarity && !owns(boon) && pick-- == 0)
        {
            gauntletBoons_grant(boon);
            return;
        }
    }
}

u8 gauntletBoons_rarity(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? Boons[boon].rarity : RARITY_COMMON;
}

const char *gauntletBoons_rarityName(u8 boon)
{
    return boon < GAUNTLET_BOON_COUNT ? RarityNames[Boons[boon].rarity] : "";
}

u32 gauntletBoons_ownedCount(void)
{
    u32 count = 0;
    u32 boon;

    for (boon = 0; boon < GAUNTLET_BOON_COUNT; boon++)
    {
        count += owns(boon);
    }
    return count;
}

static bool32 offered(const u8 offers[GAUNTLET_OFFER_COUNT], u32 filled, u8 boon)
{
    u32 i;

    for (i = 0; i < filled; i++)
    {
        if (offers[i] == boon)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void gauntletBoons_roll(u8 offers[GAUNTLET_OFFER_COUNT])
{
    u32 filled = 0;
    u32 attempt;
    bool32 legendary = FALSE;

    for (attempt = 0; attempt < 64 && filled < GAUNTLET_OFFER_COUNT; attempt++)
    {
        u8 boon = gauntlet_randBelow(GAUNTLET_BOON_COUNT);
        u8 rarity = Boons[boon].rarity;

        if (owns(boon) || offered(offers, filled, boon) ||
            (legendary && rarity == RARITY_LEGENDARY) ||
            gauntlet_randBelow(RarityWeights[RARITY_COMMON]) >= RarityWeights[rarity])
        {
            continue;
        }
        legendary |= rarity == RARITY_LEGENDARY;
        offers[filled++] = boon;
    }
    while (filled < GAUNTLET_OFFER_COUNT)
    {
        offers[filled++] = GAUNTLET_NO_BOON;
    }
}

static s32 maxHp(u8 playerIndex)
{
    return PLAYER_STAT(playerIndex, STAT_MAX_HP);
}

static s32 foeMaxHp(Actor *foe)
{
    return combat_getEnemyMaxHp(foe->unitType);
}

/** Boon damage to one foe, unable to trigger other boons. */
static void boonDamage(Actor *owner, Actor *foe, s32 damage)
{
    if (damage < 1)
    {
        damage = 1;
    }
    if (damage > 0xffff)
    {
        damage = 0xffff;
    }
    Boon.busy = TRUE;
    skillExpansion_dealDamage(owner, foe, damage, HACKLIB_DAMAGE_SECONDARY, HACKLIB_ABILITY_NONE);
    Boon.busy = FALSE;
}

static void spiritRegen(u8 playerIndex)
{
    s32 cap = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    s32 spirit;

    if (++Boon.spiritTicks < TICKS_PER_SECOND)
    {
        return;
    }
    Boon.spiritTicks = 0;
    spirit = PLAYER(playerIndex).currentSpirit + cap * 3 / 100 + 1;
    PLAYER(playerIndex).currentSpirit = spirit > cap ? cap : spirit;
}

static void secondWind(u8 playerIndex, Actor *owner)
{
    Actor *foe = NULL;

    if ((Boon.spent & SPENT_SECOND_WIND) || PLAYER(playerIndex).currentHp * 5 >= maxHp(playerIndex))
    {
        return;
    }
    Boon.spent |= SPENT_SECOND_WIND;
    skillExpansion_addHp(playerIndex, maxHp(playerIndex) * 40 / 100, FALSE);
    while ((foe = hackLib_nextTargetAt(&owner->coords, owner, SECOND_WIND_RADIUS, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            hackLib_applyStun(foe, TICKS_PER_SECOND);
        }
    }
    hackLib_fxBurst(owner, FX_SHAPE_RING_SPARKLES, FX_COLOR_WHITE, FX_LAYER_FRONT);
    skillExpansion_feedback(playerIndex, "Second Wind!");
}

static void eyeOfSauron(Actor *owner)
{
    Actor *strongest = NULL;
    Actor *cursor;

    if (++Boon.eyeTicks < EYE_INTERVAL)
    {
        return;
    }
    Boon.eyeTicks = 0;
    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        if (skillExpansion_isFoe(cursor) &&
            (strongest == NULL || cursor->currentHp > strongest->currentHp))
        {
            strongest = cursor;
        }
    }
    if (strongest != NULL)
    {
        hackLib_fxBurst(strongest, FX_SHAPE_LIGHTNING, FX_COLOR_EMBER, FX_LAYER_FRONT);
        boonDamage(owner, strongest, foeMaxHp(strongest) / 4 + 10);
    }
}

void gauntletBoons_tick(u8 playerIndex, Actor *owner, u32 room)
{
    (void)room;
    if (owns(GAUNTLET_BOON_DEEP_BREATH))
    {
        spiritRegen(playerIndex);
    }
    if (owns(GAUNTLET_BOON_SECOND_WIND))
    {
        secondWind(playerIndex, owner);
    }
    if (owns(GAUNTLET_BOON_EYE_OF_SAURON))
    {
        eyeOfSauron(owner);
    }
    if (Boon.lastStandTicks != 0 && --Boon.lastStandTicks == 0)
    {
        owner->flags.p &= ~ACTOR_FLAG_INVULNERABLE;
    }
}

s32 gauntlet_damageDealt(u8 playerIndex, Actor *victim, s32 damage)
{
    if (!gauntlet_isRunning() || Boon.busy)
    {
        return damage;
    }
    if (owns(GAUNTLET_BOON_EDGE_OF_ANDURIL))
    {
        damage = damage * 115 / 100;
    }
    if (owns(GAUNTLET_BOON_BLOOD_PRICE) && PLAYER(playerIndex).currentHp >= maxHp(playerIndex))
    {
        damage = damage * 125 / 100;
    }
    if (owns(GAUNTLET_BOON_EXECUTIONER) && victim != NULL &&
        victim->currentHp * 4 < foeMaxHp(victim))
    {
        damage *= 3;
    }
    if (Boon.lastStandTicks != 0)
    {
        damage *= 3;
    }
    return damage;
}

static void chainLightning(Actor *owner, const HackLibCombatEvent *event)
{
    Actor *foe = NULL;
    u32 arcs = 0;

    if (gauntlet_randBelow(100) >= 15)
    {
        return;
    }
    while (arcs < 2 && (foe = hackLib_nextTargetAt(&event->victim->coords, event->victim,
                                                   CHAIN_RADIUS, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_BLUE, FX_LAYER_FRONT);
        boonDamage(owner, foe, event->result.damageDealt / 2);
        arcs++;
    }
}

static void corpseBlast(Actor *owner, const HackLibCombatEvent *event)
{
    Actor *foe = NULL;
    s32 damage = foeMaxHp(event->victim) / 2;

    hackLib_fxAt(owner, &event->victim->coords, FX_SHAPE_POOL_FLASH, FX_COLOR_EMBER);
    while ((foe = hackLib_nextTargetAt(&event->victim->coords, event->victim, CORPSE_RADIUS,
                                       foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            boonDamage(owner, foe, damage);
        }
    }
}

static void hasteLoop(u8 playerIndex)
{
    u16 *cooldowns = skillExpansion_runtime[playerIndex].cooldowns;
    u32 row;

    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        cooldowns[row] =
            cooldowns[row] > HASTE_LOOP_FRAMES ? cooldowns[row] - HASTE_LOOP_FRAMES : 0;
    }
}

void gauntlet_combatEvent(u8 playerIndex, const HackLibCombatEvent *event)
{
    Actor *owner = PLAYER(playerIndex).ownerActor;

    if (gauntlet_isRunning() && event->victim != NULL && event->result.killed)
    {
        gauntlet_noteKill();
    }
    if (!gauntlet_isRunning() || Boon.busy || owner == NULL || event->victim == NULL ||
        !event->result.hit)
    {
        return;
    }
    if (owns(GAUNTLET_BOON_CHAIN_LIGHTNING) && event->result.damageDealt > 0)
    {
        chainLightning(owner, event);
    }
    if (!event->result.killed)
    {
        return;
    }
    if (owns(GAUNTLET_BOON_BLOODTHIRST))
    {
        skillExpansion_addHp(playerIndex, maxHp(playerIndex) * 2 / 100 + 1, FALSE);
    }
    if (owns(GAUNTLET_BOON_HASTE_LOOP))
    {
        hasteLoop(playerIndex);
    }
    if (owns(GAUNTLET_BOON_CORPSE_BLAST))
    {
        corpseBlast(owner, event);
    }
}

void gauntlet_enemyHitResolved(u8 playerIndex, s32 hpBefore)
{
    Actor *owner = PLAYER(playerIndex).ownerActor;

    if (!gauntlet_isRunning() || PLAYER(playerIndex).currentHp >= hpBefore)
    {
        return;
    }
    if (owns(GAUNTLET_BOON_HOBBIT_LUCK) && gauntlet_randBelow(10) == 0)
    {
        PLAYER(playerIndex).currentHp = hpBefore;
        return;
    }
    if (owns(GAUNTLET_BOON_LAST_STAND) && PLAYER(playerIndex).currentHp <= 0 &&
        !(Boon.spent & SPENT_LAST_STAND) && owner != NULL)
    {
        Boon.spent |= SPENT_LAST_STAND;
        PLAYER(playerIndex).currentHp = 1;
        Boon.lastStandTicks = LAST_STAND_TICKS;
        owner->flags.p |= ACTOR_FLAG_INVULNERABLE;
        hackLib_fxBurst(owner, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
        skillExpansion_feedback(playerIndex, "Last Stand!");
    }
}
