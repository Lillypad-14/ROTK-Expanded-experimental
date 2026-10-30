// Legolas's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

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

// Overdraw, Suppress and Summon Gimli: none yet
const LegacyActive LegacyLegolasActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [2] = {&SpreadFireRow, 9, castSpreadFire},
    [3] = {&WhiteKnivesRow, 2, castWhiteKnives},
    [4] = {&FriendOfMirkwood, 12}, // Forest Friend
};

#endif
