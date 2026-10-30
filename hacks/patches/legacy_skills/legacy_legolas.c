// Legolas's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "rng.h"
#include "stats.h"

// White Knives (Two Towers 0x0803B98C, 0x0801DA0C, the knives 0x08025C70): for 2 Spirit, Legolas
// slashes with his knives as his weapon. The Two Towers adds no damage of its own (the description's
// 4 + 1 per level is the knives'). It is ROTK's White Knives, which hits with the backpack knife as
// the weapon, without ROTK's stored damage.
static void whiteKnives(Actor *playerActor)
{
    skill_active_legolasWhiteKnives(playerActor);
}

static void castWhiteKnives(Actor *playerActor)
{
    skillExpansion_castStockCallback(playerActor);
    playerActor->as.combat.storedDamage = 0;
}

// Spread Fire (Two Towers 0x0803B934, 0x0801DEE0, an arrow 0x08026FA0, its tick 0x08027194 and hit
// 0x08010978): for 9 Spirit, 2 arrows + 1 per 4 levels (at most 7), fanned out by the angles below
// (32-way steps from Legolas's facing), each flying 21 ticks at speed 7.5 from a step ahead. An arrow
// hits the first foe it touches with Legolas's own attack, +1 accuracy per level, and is gone. Not
// ported: the glow on Legolas while he draws.
#define SPREAD_FIRE_ARROWS_BASE 2
#define SPREAD_FIRE_LEVELS_PER_ARROW 4
#define SPREAD_FIRE_ARROWS_MAX 7

static const s8 SpreadFireAngles[SPREAD_FIRE_ARROWS_MAX] = {-2, 2, 0, -4, 4, -1, 1};

static const u32 ArrowSprites[8] = {
    0x08329960u, 0x08329968u, 0x08329970u, 0x08329978u,
    0x08329980u, 0x08329978u, 0x08329970u, 0x08329968u,
};

static const LegacyThrownDef SpreadFireArrow = {
    .sprites = ArrowSprites,
    .speed = 0x78000,
    .lifetime = 21,
    .bonus = {0, 1},
    .bonusStat = STAT_ACCURACY,
    .flags = LEGACY_THROWN_STOP_ON_HIT | LEGACY_THROWN_START_AHEAD,
    .sound = LEGACY_SOUND_NONE,
};

static void castSpreadFire(Actor *playerActor)
{
    u32 arrows = SPREAD_FIRE_ARROWS_BASE + legacyFx_castingLevel(playerActor) / SPREAD_FIRE_LEVELS_PER_ARROW;

    if (arrows > SPREAD_FIRE_ARROWS_MAX)
    {
        arrows = SPREAD_FIRE_ARROWS_MAX;
    }
    for (u32 i = 0; i < arrows; i++)
    {
        legacyFx_castThrownToward(playerActor, &SpreadFireArrow, playerActor->facing * 4 + SpreadFireAngles[i]);
    }
}

// Suppress (Two Towers 0x0803C404, the arrow 0x0802730C, its hit 0x08011598; the knock 0x080274E4,
// its tick 0x08027624 and hit 0x08011754): for 3 Spirit, an arrow like Spread Fire's that hits the
// first foe it touches with Legolas's attack and is gone. Where it hits, a knock touches the foes
// within 35 pixels once, each knocked back along the arrow for 15 ticks on a roll of 0..100 at most
// 25 + 3 per level (the description says 25% + 5% per level). Not ported: the glow on Legolas.
#define SUPPRESS_RADIUS 35
#define SUPPRESS_KNOCK_TICKS 15
#define SUPPRESS_KNOCK 0x08052660u

static const LegacyScale SuppressChance = {25, 3};

static const u32 SuppressSprites[8] = {
    0x083299B0u, 0x083299B8u, 0x083299C0u, 0x083299C8u,
    0x083299D0u, 0x083299C8u, 0x083299C0u, 0x083299B8u,
};

static void knockTick(Actor *fx)
{
    Actor *foe = LEGACY_FX(fx)->target;

    // it knocks on the frame it appears only
    fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    if (foe != NULL)
    {
        fx->coords = foe->coords;
    }
    fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : SUPPRESS_KNOCK_TICKS;
    actor_unknown08045584(fx);
}

static void knockHit(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other) && rng_rollRange(0, 100) <= LEGACY_FX(fx)->amount)
    {
        actor_applyKnockback(other, fx, SUPPRESS_KNOCK_TICKS);
    }
}

static void suppressKnock(Actor *arrow, Actor *foe)
{
    Actor *fx = legacyFx_spawn(arrow->parentActor, SUPPRESS_KNOCK);

    if (fx == NULL)
    {
        return;
    }
    fx->coords = foe->coords;
    fx->facing = arrow->facing;
    fx->tickCallback = knockTick;
    fx->triggerCallbacks[0] = knockHit;
    fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY | ACTOR_FLAG_COLLISION_CHECK;
    fx->interactionRadius = SUPPRESS_RADIUS;
    fx->actionStateTimer = SUPPRESS_KNOCK_TICKS;
    LEGACY_FX(fx)->target = foe;
    LEGACY_FX(fx)->amount = legacyScale(SuppressChance, legacyFx_castingLevel(arrow->parentActor));
}

static const LegacyThrownDef SuppressArrow = {
    .sprites = SuppressSprites,
    .speed = 0x78000,
    .lifetime = 21,
    .flags = LEGACY_THROWN_STOP_ON_HIT | LEGACY_THROWN_START_AHEAD,
    .sound = LEGACY_SOUND_NONE,
    .onHit = suppressKnock,
};

static void castSuppress(Actor *playerActor)
{
    legacyFx_castThrown(playerActor, &SuppressArrow);
}

// ROTK's rows the actives take their cast poses from
// ROTK's White Knives needs a knife in the backpack; Legolas always has his in The Two Towers
static const SkillExpansionCastPolicy WhiteKnivesPolicy = {
    ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES + 1, ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionActiveDef WhiteKnivesRow = {
    .stockCallback = whiteKnives,
    .baseRow = ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES,
    .policy = &WhiteKnivesPolicy,
};
static const SkillExpansionActiveDef SpreadFireRow = {.baseRow = ACTIVE_SKILL_LEGOLAS_SPREAD_FIRE};
// a stand-in: ROTK's Friend of Mirkwood
static const SkillExpansionActiveDef FriendOfMirkwood = {
    .stockCallback = skill_active_legolasFriendOfMirkwood,
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
};

// Overdraw and Summon Gimli: none yet
const LegacyActive LegacyLegolasActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [1] = {&SpreadFireRow, 3, castSuppress},
    [2] = {&SpreadFireRow, 9, castSpreadFire},
    [3] = {&WhiteKnivesRow, 2, castWhiteKnives},
    [4] = {&FriendOfMirkwood, 12}, // Forest Friend
};

#endif
