#include "patches/skill_expansion/skill_expansion_eowyn.h"
#include "enemy.h"
#include "fx.h"
#include "game.h"
#include "lib/actor_ref.h"
#include "lib/combat_source.h"
#include "lib/fx_kit.h"
#include "lib/hud_meter.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/summon.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "sfx.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

#define FRAMES_PER_SECOND 30

// Cast visuals step every CAST_FX_TICKS so they read instead of flashing by.
#define CAST_FX_TICKS 3

#define ODDS_UNLOCK_LEVEL 10

// 1/sqrt(2) scaled by 256, for diagonal facings
#define DIAGONAL_SCALE 181

// Stock Eowyn sounds
#define SFX_FORTH_EORLINGAS 0x58
#define SFX_ROHAN_SPRINT 0x68
#define SFX_SHIELD_BASH 0x69

// Odds, recomputed every ODDS_FRAMES
#define ODDS_FRAMES 15
#define ODDS_RADIUS 80
#define ODDS_MAX_NEAR 5
#define ODDS_NAZGUL 3
#define ODDS_SCREECH 2
#define ODDS_PARTNER 2
#define ODDS_PARTNER_HP_PERCENT 25
#define SCREECH_FRAMES (5 * FRAMES_PER_SECOND)

// Tiers. A lower tier only takes hold after TIER_FALL_FRAMES.
#define TIER_STEADY 0
#define TIER_GRIM 1
#define TIER_DESPERATE 2
#define TIER_HOPELESS 3
#define TIER_COUNT 4
#define TIER_FALL_FRAMES (2 * FRAMES_PER_SECOND)

// Eowyn glows brighter per tier.
#define EOWYN_OBJ_PALETTE (0x21 + HERO_ID_EOWYN)
#define GLOW_REFRESH_FRAMES 16
#define HOPELESS_SPARKLE_FRAMES (2 * FRAMES_PER_SECOND)

// Gallop
#define GALLOP_MAX 5
#define GALLOP_STEP_PROGRESS (40 * 100)
#define GALLOP_SPEED_PER_LEVEL 15
#define GALLOP_STILL_FRAMES FRAMES_PER_SECOND
#define GALLOP_DECAY_FRAMES FRAMES_PER_SECOND
#define GALLOP_PERCENT_PER_STEP 15
#define GALLOP_KNOCKBACK_STEPS 3
#define GALLOP_KNOCKBACK 16
#define SPRINT_GALLOP_STEPS 2
#define SPRINT_GALLOP_FRAMES (5 * FRAMES_PER_SECOND)
#define BAR_FLASH_FRAMES 8

// Page 1 ties
#define EORLINGAS_FRAMES (8 * FRAMES_PER_SECOND)
#define SHIELD_BASH_FRAMES_PER_TIER 7

// Death-Seeker's Strike (Despair)
#define DEATH_SEEKER_REACH 40
#define DEATH_SEEKER_HP_PERCENT 10
#define DEATH_SEEKER_MIN_HP_PERCENT 5
#define DEATH_SEEKER_FLOOR_PERCENT 15
#define DEATH_SEEKER_BASE_DAMAGE 14
#define DEATH_SEEKER_DAMAGE_PER_LEVEL 6
#define DEATH_SEEKER_PERCENT_PER_TIER 25

// Come and Face Me! (Shieldmaiden)
#define FACE_RADIUS 96
#define FACE_PULL 24
#define FACE_CLOSEST 16
#define FACE_STUN_FRAMES (FRAMES_PER_SECOND / 2)
#define FACE_FRAMES (4 * FRAMES_PER_SECOND)
#define FACE_PERCENT_PER_FOE 5
#define FACE_MAX_PERCENT 30

// Lance of the Mark (Rohirrim)
#define LANCE_LUNGE 32
#define LANCE_LENGTH 64
#define LANCE_HALF_WIDTH 16
#define LANCE_BASE_DAMAGE 12
#define LANCE_DAMAGE_PER_LEVEL 6
#define LANCE_PERCENT_PER_STEP 30
#define LANCE_PERCENT_PER_TIER 20
#define LANCE_KNOCKBACK 32

// Stand Over the Fallen (Shieldmaiden)
#define STAND_BASE_FRAMES (3 * FRAMES_PER_SECOND)
#define STAND_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define STAND_BASE_REFLECT_PERCENT 40
#define STAND_REFLECT_PERCENT_PER_LEVEL 5
#define STAND_GUARD_RADIUS 48
#define STAND_GUARD_TAKEN_PERCENT 70

// Hope Rekindled (Rohirrim), per mille of max HP per Odds point
#define HOPE_BASE_PER_MILLE 20
#define HOPE_PER_MILLE_PER_LEVEL 5
#define HOPE_MAX_ODDS 12

// I Am No Man! (ultimate)
#define NO_MAN_RADIUS 64
#define NO_MAN_STUN_FRAMES (3 * FRAMES_PER_SECOND / 2)
#define NO_MAN_BASE_FRAMES (8 * FRAMES_PER_SECOND)
#define NO_MAN_FRAMES_PER_LEVEL (2 * FRAMES_PER_SECOND)
#define NO_MAN_PERCENT 100

// Passives
#define UNBOWED_PERCENT_PER_LEVEL 3
#define NOTHING_LEFT_PERCENT_PER_LEVEL 2
#define HOLD_FAST_BASE_PERCENT 30
#define HOLD_FAST_PERCENT_PER_LEVEL 3
#define TRAMPLE_STEPS 3
#define TRAMPLE_RADIUS 14
#define TRAMPLE_BASE_DAMAGE 4
#define TRAMPLE_DAMAGE_PER_LEVEL 2
#define TRAMPLE_STUN_FRAMES 10
#define TRAMPLE_COOLDOWN_FRAMES 10
#define RUIN_RADIUS 48
#define RUIN_HOPELESS_RADIUS 64
#define RUIN_BASE_DAMAGE 10
#define RUIN_STUN_FRAMES FRAMES_PER_SECOND
#define DAWN_HEAL_PERCENT 30
#define DAWN_SPIRIT 50
#define DAWN_COOLDOWN_FRAMES (30 * FRAMES_PER_SECOND)
#define DOOM_PERCENT 100
#define DOOM_TAKEN_PERCENT 50
#define MAX_TAKEN_CUT_PERCENT 75

static const u8 TierDamagePercents[TIER_COUNT] = {0, 10, 20, 35};
static const u8 TierTakenCutPercents[TIER_COUNT] = {0, 0, 10, 20};
static const char *const TierMessages[TIER_COUNT] = {"Steady", "Grim", "Desperate",
                                                     "Hopeless!"};

#define GLOW_COLOR(r, g, b) ((r) | (g) << 5 | (b) << 10)
static const u16 TierGlowColors[TIER_COUNT] = {
    GLOW_COLOR(31, 31, 31),
    GLOW_COLOR(31, 26, 8),
    GLOW_COLOR(31, 16, 4),
    GLOW_COLOR(31, 31, 22),
};
static const u8 TierGlowStrengths[TIER_COUNT] = {0, 3, 6, 10};
static const HackLibFxColor TierFxColors[TIER_COUNT] = {FX_COLOR_WHITE, FX_COLOR_GOLD,
                                                        FX_COLOR_EMBER, FX_COLOR_WHITE};

typedef struct EowynState
{
    // Stand Over the Fallen root position
    Vector2Fp16 standPos;

    // foe that hit Eowyn during Stand Over the Fallen, and the damage to send back
    HackLibActorRef reflectTarget;
    u16 reflectDamage;

    // position at the previous tick in pixels
    s16 lastX;
    s16 lastY;

    // Gallop progress toward the next step
    u16 gallopProgress;

    // I Am No Man! frames left
    u16 noManFrames;

    // Stand Over the Fallen frames left
    u16 standFrames;

    // Come and Face Me! guard frames left
    u16 faceFrames;

    // Forth Eorlingas tier lift frames left
    u16 eorlingasFrames;

    // frames the last fear status keeps adding to the Odds
    u16 screechFrames;

    // frames until Dawn Over the Pelennor can heal again
    u16 dawnCooldown;

    // frames left of Rohan Sprint's faster Gallop
    u16 sprintFrames;

    // frames Eowyn has stood still, capped
    u8 stillFrames;

    // frames until the next Odds read
    u8 oddsTimer;

    // Odds at the last read
    u8 odds;

    // current tier, TIER_STEADY..TIER_HOPELESS
    u8 tier;

    // frames a lower tier has been read in a row
    u8 fallFrames;

    // Gallop steps, 0..GALLOP_MAX
    u8 gallop;

    // foes Come and Face Me! pulled in
    u8 faceFoes;

    // Stand Over the Fallen level at cast
    u8 standLevel;

    // row Stand Over the Fallen was cast from
    u8 standRow;

    // frames until Trample can hit again
    u8 trampleCooldown;

    // frames left of the Gallop meter's spend flash
    u8 barFlashFrames;

    // glow strength loaded into the sprite
    u8 glowStrength;

    // frames until the glow reloads
    u8 glowTimer;

    // frames to the next Hopeless sparkle
    u8 sparkleTimer;

    // TRUE while the player had a fear status on the previous tick
    bool feared;
} EowynState;

_Static_assert(sizeof(EowynState) <= sizeof(skillExpansion_heroState[0]),
               "EowynState exceeds the hero state");

static inline EowynState *state(u8 playerIndex)
{
    return (EowynState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_EOWYN,
                                                    PLAYER(playerIndex).heroId);
}

static const Vector2Int FacingVectors[8] = {
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
};

static inline bool isEowyn(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_EOWYN;
}

static inline bool oddsUnlocked(u8 playerIndex)
{
    return PLAYER(playerIndex).level + 1 >= ODDS_UNLOCK_LEVEL;
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline u8 castLevel(u8 playerIndex)
{
    return skillExpansion_effectiveActiveLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

static inline s32 percentOf(s32 value, s32 percent)
{
    return value * percent / 100;
}

static inline s32 pixel(s32 coordinate)
{
    return coordinate >> FP16_16_SHIFT;
}

static inline s32 maxHp(u8 playerIndex)
{
    return PLAYER_STAT(playerIndex, STAT_MAX_HP);
}

static bool gallopLearned(u8 playerIndex)
{
    return passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RIDERS_OF_THE_MARK) != 0;
}

static Actor *castFx(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(playerActor, shape, color, layer), CAST_FX_TICKS);
}

static s32 distanceSquared(const Actor *a, const Actor *b)
{
    s32 dx = pixel(a->coords.x - b->coords.x);
    s32 dy = pixel(a->coords.y - b->coords.y);

    return dx * dx + dy * dy;
}

static s32 squareRoot(s32 value)
{
    s32 root = 0;

    while ((root + 1) * (root + 1) <= value)
    {
        root++;
    }
    return root;
}

static bool isNazgul(const Actor *foe)
{
    return foe->unitType < ENEMY_TYPE_COUNT &&
           EnemyTypeInfos[foe->unitType].family == ENEMY_FAMILY_NAZGUL;
}

/** A foe with more max HP than Eowyn. */
static bool isBigger(u8 playerIndex, const Actor *foe)
{
    return foe->unitType < ENEMY_TYPE_COUNT &&
           combat_getEnemyMaxHp(foe->unitType) > maxHp(playerIndex);
}

static Actor *nearestFoe(Actor *center, s32 radius)
{
    Actor *foe = NULL;
    Actor *best = NULL;
    s32 bestDistance = radius * radius + 1;

    while ((foe = findNextEnemy(center, foe)) != NULL)
    {
        s32 distance;

        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        distance = distanceSquared(center, foe);
        if (distance < bestDistance)
        {
            best = foe;
            bestDistance = distance;
        }
    }
    return best;
}

static Vector2Int facingOf(const Actor *actor)
{
    return FacingVectors[actor->facing < 8 ? actor->facing : DIRECTION8_SOUTH];
}

/** Distance of the foe along the actor's facing line, or -1 when it is off the line. */
static s32 alongLine(const Actor *actor, const Actor *foe, s32 length)
{
    Vector2Int facing = facingOf(actor);
    s32 scale = facing.x != 0 && facing.y != 0 ? DIAGONAL_SCALE : 256;
    s32 dx = pixel(foe->coords.x) - pixel(actor->coords.x);
    s32 dy = pixel(foe->coords.y) - pixel(actor->coords.y);
    s32 along = (dx * facing.x + dy * facing.y) * scale / 256;
    s32 across = ABS(dx * facing.y - dy * facing.x) * scale / 256;

    if (along < 0 || along > length || across > LANCE_HALF_WIDTH)
    {
        return -1;
    }
    return along;
}

/**
 * Move the actor up to `distance` pixels along (dx, dy), stopping at walls. A negative distance
 * moves it the other way.
 */
static void slide(Actor *actor, s32 dx, s32 dy, s32 distance)
{
    s32 length = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
    s32 sign = distance < 0 ? -1 : 1;
    Vector2Fp16 landing = actor->coords;
    s32 step;

    if (length == 0)
    {
        return;
    }
    for (step = 1; step <= ABS(distance); step++)
    {
        Vector2Fp16 next = {actor->coords.x + sign * dx * step * FP16_16_ONE / length,
                            actor->coords.y + sign * dy * step * FP16_16_ONE / length};

        if (!hackLib_canLandAt(actor, &next))
        {
            break;
        }
        landing = next;
    }
    actor->renderCoords.x += landing.x - actor->coords.x;
    actor->renderCoords.y += landing.y - actor->coords.y;
    actor->coords = landing;
    actor->velocity.x = actor->velocity.y = 0;
}

/** Push the foe `distance` pixels straight away from the actor. */
static void knockBack(Actor *actor, Actor *foe, s32 distance)
{
    slide(foe, pixel(foe->coords.x) - pixel(actor->coords.x),
          pixel(foe->coords.y) - pixel(actor->coords.y), distance);
}

static u32 shockwave(Actor *actor, s32 radius, s32 damage, s32 stunFrames, u8 row)
{
    SkillExpansionBurst hit = {
        .attacker = actor,
        .center = actor,
        .radius = radius,
        .damage = damage > 0xffff ? 0xffff : damage,
        .stunFrames = stunFrames,
        .kind = HACKLIB_DAMAGE_ABILITY,
        .abilityRow = row,
    };

    return skillExpansion_burstDamage(&hit);
}

/** Odds read from the fight around Eowyn. */
static u32 readOdds(u8 playerIndex, EowynState *st, Actor *actor)
{
    Player *player = &PLAYER(playerIndex);
    s32 max = maxHp(playerIndex);
    Actor *foe = NULL;
    u32 nearFoes = 0;
    u32 bigFoes = 0;
    bool nazgul = FALSE;
    u32 odds;

    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        nazgul |= isNazgul(foe);
        if (distanceSquared(actor, foe) > ODDS_RADIUS * ODDS_RADIUS)
        {
            continue;
        }
        if (nearFoes < ODDS_MAX_NEAR)
        {
            nearFoes++;
            bigFoes += isBigger(playerIndex, foe);
        }
    }
    odds = nearFoes + bigFoes;
    if (nazgul)
    {
        odds += ODDS_NAZGUL;
    }
    if (st->screechFrames != 0)
    {
        odds += ODDS_SCREECH;
    }
    if (max > 0 && player->currentHp < max)
    {
        u32 quarters = (max - player->currentHp) * 4 / max;

        if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SEEKING_DEATH) != 0)
        {
            quarters *= 2;
        }
        odds += quarters;
    }
    if (g_GameFlags.d.coopActive)
    {
        Player *partner = &PLAYER(playerIndex ^ 1);
        s32 partnerMax = PLAYER_STAT(playerIndex ^ 1, STAT_MAX_HP);

        if (partner->currentHp > 0 &&
            partner->currentHp * 100 < partnerMax * ODDS_PARTNER_HP_PERCENT)
        {
            odds += ODDS_PARTNER;
        }
    }
    return odds > 0xff ? 0xff : odds;
}

static u8 tierOf(u32 odds)
{
    if (odds >= 9)
    {
        return TIER_HOPELESS;
    }
    if (odds >= 6)
    {
        return TIER_DESPERATE;
    }
    return odds >= 3 ? TIER_GRIM : TIER_STEADY;
}

static void loadGlow(Actor *actor, u8 tier, u8 strength)
{
    hackLib_fxTintSlot(actor->oam.paletteNumber, ObjPalettes[EOWYN_OBJ_PALETTE], 1, 15,
                       TierGlowColors[tier], strength);
}

static void showGlow(EowynState *st, Actor *actor)
{
    u8 strength = TierGlowStrengths[st->tier];

    if (strength == st->glowStrength && (strength == 0 || st->glowTimer != 0))
    {
        if (st->glowTimer != 0)
        {
            st->glowTimer--;
        }
        return;
    }
    st->glowStrength = strength;
    st->glowTimer = GLOW_REFRESH_FRAMES;
    loadGlow(actor, st->tier, strength);
}

static void dawnOverThePelennor(u8 playerIndex, EowynState *st, Actor *actor)
{
    s32 heal = percentOf(maxHp(playerIndex), DAWN_HEAL_PERCENT);

    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DAWN_OVER_THE_PELENNOR) == 0 ||
        st->dawnCooldown != 0)
    {
        return;
    }
    st->dawnCooldown = DAWN_COOLDOWN_FRAMES;
    player_addHp(playerIndex, heal > 0 ? heal : 1, FALSE);
    player_addSpirit(playerIndex, DAWN_SPIRIT);
    castFx(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GOLD, FX_LAYER_FRONT);
    castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
    sfx_play(SFX_SKILL_TREE_BUY_ACTIVE);
    skillExpansion_feedback(playerIndex, "The dawn has come!");
}

static void setTier(u8 playerIndex, EowynState *st, Actor *actor, u8 tier)
{
    u8 previous = st->tier;

    st->tier = tier;
    st->fallFrames = 0;
    if (tier > previous)
    {
        castFx(actor, tier == TIER_HOPELESS ? FX_SHAPE_TENDRIL_STAR : FX_SHAPE_SPARKLE_RING,
               TierFxColors[tier], FX_LAYER_FRONT);
        sfx_play(tier == TIER_HOPELESS ? SFX_COMBAT_CRIT_HIT : SFX_SKILL_TREE_BUY_PASSIVE);
        skillExpansion_feedback(playerIndex, TierMessages[tier]);
    }
    else if (previous == TIER_HOPELESS && tier == TIER_STEADY)
    {
        dawnOverThePelennor(playerIndex, st, actor);
    }
}

static void tickOdds(u8 playerIndex, EowynState *st, Actor *actor)
{
    u8 tier;

    if (st->oddsTimer != 0)
    {
        st->oddsTimer--;
        return;
    }
    st->oddsTimer = ODDS_FRAMES - 1;
    st->odds = readOdds(playerIndex, st, actor);
    tier = tierOf(st->odds);
    if (st->eorlingasFrames != 0 && tier < TIER_HOPELESS)
    {
        tier++;
    }
    if (st->noManFrames != 0)
    {
        tier = TIER_HOPELESS;
    }
    if (tier > st->tier)
    {
        setTier(playerIndex, st, actor, tier);
    }
    else if (tier < st->tier)
    {
        st->fallFrames += ODDS_FRAMES;
        if (st->fallFrames >= TIER_FALL_FRAMES)
        {
            setTier(playerIndex, st, actor, tier);
        }
    }
    else
    {
        st->fallFrames = 0;
    }
}

/** A fear status on Eowyn adds to the Odds. Unbowed and No Living Man shrug it off. */
static void tickFear(u8 playerIndex, EowynState *st, Actor *actor)
{
    bool feared = actor->as.combat.fearDuration > 0;

    if (feared && !st->feared)
    {
        st->screechFrames = SCREECH_FRAMES;
    }
    st->feared = feared;
    if (feared && actor->as.combat.fearDuration > 1 &&
        (st->noManFrames != 0 ||
         passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNBOWED) != 0 ||
         passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DOOM_OF_THE_WITCH_KING) != 0))
    {
        // The stock status tick ends the fear and its HUD tint on the next frame.
        actor->as.combat.fearDuration = 1;
    }
}

static bool gallopLocked(u8 playerIndex, EowynState *st)
{
    return st->noManFrames != 0 && gallopLearned(playerIndex);
}

static void gainStep(u8 playerIndex, EowynState *st, Actor *actor)
{
    if (st->gallop >= GALLOP_MAX)
    {
        return;
    }
    st->gallop++;
    hackLib_fxAt(actor, &actor->coords, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE);
    if (st->gallop == GALLOP_MAX)
    {
        castFx(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_BEHIND);
        sfx_play(SFX_ROHAN_SPRINT);
        skillExpansion_feedback(playerIndex, "Full gallop!");
    }
}

static void trample(u8 playerIndex, EowynState *st, Actor *actor)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_TRAMPLE);
    Actor *foe;
    HackLibDamageResult result;

    if (st->trampleCooldown != 0)
    {
        st->trampleCooldown--;
        return;
    }
    if (level == 0 || st->gallop < TRAMPLE_STEPS)
    {
        return;
    }
    foe = nearestFoe(actor, TRAMPLE_RADIUS);
    if (foe == NULL)
    {
        return;
    }
    st->trampleCooldown = TRAMPLE_COOLDOWN_FRAMES;
    result = skillExpansion_dealDamage(actor, foe,
                                       TRAMPLE_BASE_DAMAGE + TRAMPLE_DAMAGE_PER_LEVEL * level,
                                       HACKLIB_DAMAGE_ABILITY, HACKLIB_ABILITY_NONE);
    if (result.hit && !result.killed)
    {
        hackLib_applyStun(foe, TRAMPLE_STUN_FRAMES);
    }
    hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
}

static void tickGallop(u8 playerIndex, EowynState *st, Actor *actor)
{
    s32 x = pixel(actor->coords.x);
    s32 y = pixel(actor->coords.y);
    bool moving = x != st->lastX || y != st->lastY;

    st->lastX = x;
    st->lastY = y;
    if (st->barFlashFrames != 0)
    {
        st->barFlashFrames--;
    }
    if (!gallopLearned(playerIndex))
    {
        st->gallop = 0;
        return;
    }
    if (gallopLocked(playerIndex, st))
    {
        st->gallop = GALLOP_MAX;
        return;
    }
    if (moving)
    {
        u32 gain = 100 + GALLOP_SPEED_PER_LEVEL *
                             passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RIDERS_OF_THE_MARK);

        if (st->sprintFrames != 0)
        {
            gain *= 2;
        }
        if (st->tier >= TIER_DESPERATE)
        {
            gain *= 2;
        }
        st->stillFrames = 0;
        st->gallopProgress += gain;
        if (st->gallopProgress >= GALLOP_STEP_PROGRESS)
        {
            st->gallopProgress -= GALLOP_STEP_PROGRESS;
            gainStep(playerIndex, st, actor);
        }
        trample(playerIndex, st, actor);
        return;
    }
    st->gallopProgress = 0;
    if (st->trampleCooldown != 0)
    {
        st->trampleCooldown--;
    }
    if (++st->stillFrames >= GALLOP_STILL_FRAMES + GALLOP_DECAY_FRAMES)
    {
        st->stillFrames = GALLOP_STILL_FRAMES;
        if (st->gallop != 0)
        {
            st->gallop--;
        }
    }
}

static void tickStand(EowynState *st, Actor *actor)
{
    if (st->standFrames == 0)
    {
        return;
    }
    actor->renderCoords.x += st->standPos.x - actor->coords.x;
    actor->renderCoords.y += st->standPos.y - actor->coords.y;
    actor->coords = st->standPos;
    actor->velocity.x = actor->velocity.y = 0;
    st->standFrames--;
}

/** Send back the damage Stand Over the Fallen stored, outside the incoming-damage path. */
static void tickReflect(EowynState *st, Actor *actor)
{
    Actor *foe = st->reflectTarget.actor;
    u16 damage = st->reflectDamage;

    if (damage == 0)
    {
        return;
    }
    st->reflectDamage = 0;
    if (!hackLib_isActorRefValid(st->reflectTarget) || !skillExpansion_isFoe(foe))
    {
        return;
    }
    skillExpansion_dealDamage(actor, foe, damage, HACKLIB_DAMAGE_ABILITY, st->standRow);
    hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
}

static void clearState(u8 playerIndex, Actor *actor)
{
    EowynState *st = state(playerIndex);

    if (actor != NULL && st->glowStrength != 0)
    {
        loadGlow(actor, TIER_STEADY, 0);
    }
    memory_memset(st, 0, sizeof(*st));
    if (actor != NULL)
    {
        st->lastX = pixel(actor->coords.x);
        st->lastY = pixel(actor->coords.y);
    }
}

void skillExpansion_eowynReset(u8 playerIndex)
{
    if (!isEowyn(playerIndex))
    {
        return;
    }
    clearState(playerIndex, PLAYER(playerIndex).ownerActor);
}

void skillExpansion_eowynCleanup(u8 playerIndex, Actor *actor)
{
    if (!isEowyn(playerIndex))
    {
        return;
    }
    clearState(playerIndex, actor);
}

void skillExpansion_eowynTick(u8 playerIndex, Actor *actor)
{
    EowynState *st;

    if (!isEowyn(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (st->noManFrames != 0 && --st->noManFrames == 0)
    {
        if (gallopLearned(playerIndex))
        {
            st->gallop = 0;
        }
        skillExpansion_feedback(playerIndex, "The helm is set aside.");
    }
    if (st->faceFrames != 0 && --st->faceFrames == 0)
    {
        st->faceFoes = 0;
    }
    if (st->eorlingasFrames != 0)
    {
        st->eorlingasFrames--;
    }
    if (st->screechFrames != 0)
    {
        st->screechFrames--;
    }
    if (st->dawnCooldown != 0)
    {
        st->dawnCooldown--;
    }
    if (st->sprintFrames != 0)
    {
        st->sprintFrames--;
    }
    tickStand(st, actor);
    tickReflect(st, actor);
    if (!oddsUnlocked(playerIndex))
    {
        return;
    }
    tickFear(playerIndex, st, actor);
    tickOdds(playerIndex, st, actor);
    tickGallop(playerIndex, st, actor);
    showGlow(st, actor);
    if (st->tier == TIER_HOPELESS)
    {
        if (st->sparkleTimer == 0)
        {
            st->sparkleTimer = HOPELESS_SPARKLE_FRAMES;
            hackLib_fxBurst(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_WHITE, FX_LAYER_FRONT);
        }
        st->sparkleTimer--;
    }
}

s32 skillExpansion_eowynDamage(u8 playerIndex, Actor *victim, s32 damage, bool melee)
{
    EowynState *st;
    s32 percent;

    if (!isEowyn(playerIndex) || victim == NULL || damage <= 0 || !oddsUnlocked(playerIndex))
    {
        return damage;
    }
    st = state(playerIndex);
    percent = TierDamagePercents[st->tier];
    if (melee && st->gallop != 0 && gallopLearned(playerIndex))
    {
        percent += GALLOP_PERCENT_PER_STEP * st->gallop;
    }
    if (st->noManFrames != 0 && (isNazgul(victim) || isBigger(playerIndex, victim)))
    {
        percent += NO_MAN_PERCENT;
    }
    if (isNazgul(victim) &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DOOM_OF_THE_WITCH_KING) != 0)
    {
        percent += DOOM_PERCENT;
    }
    return damage + percentOf(damage, percent);
}

/** Stand Over the Fallen: Eowyn takes a hit meant for her partner while near. */
static s32 guardPartner(u8 victim, s32 damage)
{
    u8 eowyn = victim ^ 1;
    Actor *guard = PLAYER(eowyn).ownerActor;
    Actor *ward = PLAYER(victim).ownerActor;
    s32 taken;

    if (!g_GameFlags.d.coopActive || !isEowyn(eowyn) || guard == NULL || ward == NULL ||
        PLAYER(eowyn).currentHp <= 1 || state(eowyn)->standFrames == 0 ||
        distanceSquared(guard, ward) > STAND_GUARD_RADIUS * STAND_GUARD_RADIUS)
    {
        return damage;
    }
    taken = percentOf(damage, STAND_GUARD_TAKEN_PERCENT);
    if (taken >= PLAYER(eowyn).currentHp)
    {
        taken = PLAYER(eowyn).currentHp - 1;
    }
    if (taken > 0)
    {
        skillExpansion_addHp(eowyn, -taken, FALSE);
    }
    hackLib_fxBurst(guard, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
    return 0;
}

s32 skillExpansion_eowynIncomingDamage(u8 playerIndex, Actor *attacker, s32 damage)
{
    EowynState *st;
    s32 unbowed;
    s32 percent;

    if (damage <= 0 || playerIndex >= 2)
    {
        return damage;
    }
    if (!isEowyn(playerIndex))
    {
        return guardPartner(playerIndex, damage);
    }
    st = state(playerIndex);
    if (st->standFrames != 0 && attacker != NULL && skillExpansion_isFoe(attacker))
    {
        u32 reflect = st->reflectDamage +
                      percentOf(damage, STAND_BASE_REFLECT_PERCENT +
                                            STAND_REFLECT_PERCENT_PER_LEVEL * st->standLevel);

        st->reflectTarget = hackLib_actorRef(attacker);
        st->reflectDamage = reflect > 0xffff ? 0xffff : reflect;
    }
    if (!gallopLocked(playerIndex, st) && st->gallop != 0)
    {
        st->gallop--;
    }
    if (!oddsUnlocked(playerIndex))
    {
        return damage;
    }
    unbowed = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNBOWED);
    percent = TierTakenCutPercents[st->tier] + UNBOWED_PERCENT_PER_LEVEL * unbowed * st->tier;
    if (st->faceFrames != 0)
    {
        s32 guard = FACE_PERCENT_PER_FOE * st->faceFoes;

        percent += guard > FACE_MAX_PERCENT ? FACE_MAX_PERCENT : guard;
    }
    if (attacker != NULL && attacker->type == ACTOR_TYPE_ENEMY && isNazgul(attacker) &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DOOM_OF_THE_WITCH_KING) != 0)
    {
        percent += DOOM_TAKEN_PERCENT;
    }
    damage -= percentOf(damage, percent > MAX_TAKEN_CUT_PERCENT ? MAX_TAKEN_CUT_PERCENT : percent);
    if (st->tier == TIER_HOPELESS)
    {
        s32 holdFast = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HOLD_FAST);
        s32 cap = percentOf(maxHp(playerIndex),
                            HOLD_FAST_BASE_PERCENT - HOLD_FAST_PERCENT_PER_LEVEL * holdFast);

        if (holdFast != 0 && damage > cap)
        {
            damage = cap;
        }
    }
    return damage < 1 ? 1 : damage;
}

bool skillExpansion_eowynFlinch(u8 playerIndex, bool flinchOnDamage)
{
    EowynState *st;

    if (!isEowyn(playerIndex))
    {
        return flinchOnDamage;
    }
    st = state(playerIndex);
    if (st->standFrames != 0 ||
        (st->tier == TIER_HOPELESS &&
         passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HOLD_FAST) != 0))
    {
        return FALSE;
    }
    return flinchOnDamage;
}

/** The hit spends Gallop: knockback at 3+ steps, Ride to Ruin at 5. */
static void spendGallop(u8 playerIndex, EowynState *st, Actor *actor, Actor *victim)
{
    u8 steps = st->gallop;

    if (steps == 0 || !gallopLearned(playerIndex))
    {
        return;
    }
    if (steps >= GALLOP_KNOCKBACK_STEPS && victim->currentHp > 0)
    {
        knockBack(actor, victim, GALLOP_KNOCKBACK);
    }
    hackLib_fxBurst(victim, steps >= GALLOP_MAX ? FX_SHAPE_STAR_BLOOM : FX_SHAPE_SPARK,
                    FX_COLOR_GOLD, FX_LAYER_FRONT);
    if (gallopLocked(playerIndex, st))
    {
        return;
    }
    st->gallop = 0;
    st->gallopProgress = 0;
    st->barFlashFrames = BAR_FLASH_FRAMES;
    if (steps >= GALLOP_MAX &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RIDE_TO_RUIN) != 0)
    {
        s32 damage = RUIN_BASE_DAMAGE + PLAYER(playerIndex).level + 1;

        damage += percentOf(damage, GALLOP_PERCENT_PER_STEP * GALLOP_MAX);
        castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GOLD, FX_LAYER_BEHIND);
        sfx_play(SFX_FORTH_EORLINGAS);
        shockwave(actor, st->tier == TIER_HOPELESS ? RUIN_HOPELESS_RADIUS : RUIN_RADIUS, damage,
                  RUIN_STUN_FRAMES, HACKLIB_ABILITY_NONE);
        skillExpansion_feedback(playerIndex, "Ride to ruin!");
    }
}

void skillExpansion_eowynOnHit(u8 playerIndex, Actor *attacker, Actor *victim,
                               HackLibDamageResult result, bool melee)
{
    EowynState *st;
    s32 nothingLeft;

    if (!isEowyn(playerIndex) || attacker == NULL || victim == NULL || !result.hit ||
        !oddsUnlocked(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    nothingLeft = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_NOTHING_LEFT_TO_LOSE);
    if (nothingLeft != 0 && st->tier >= TIER_DESPERATE && result.damageDealt > 0)
    {
        s32 heal = percentOf(result.damageDealt, NOTHING_LEFT_PERCENT_PER_LEVEL * nothingLeft);

        player_addHp(playerIndex, heal > 0 ? heal : 1, FALSE);
    }
    if (!melee)
    {
        return;
    }
    if (attacker->actionState == ACTOR_STATE_EOWYN_SHIELD_BASH &&
        victim->actionState == ACTOR_STATE_STUNNED && st->tier != TIER_STEADY)
    {
        victim->actionStateTimer += SHIELD_BASH_FRAMES_PER_TIER * st->tier;
    }
    spendGallop(playerIndex, st, attacker, victim);
}

static void rohanSprint(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    skill_active_eowynRohanSprint(playerActor);
    if (isEowyn(p) && oddsUnlocked(p) && gallopLearned(p))
    {
        EowynState *st = state(p);
        u32 i;

        st->sprintFrames = SPRINT_GALLOP_FRAMES;
        for (i = 0; i < SPRINT_GALLOP_STEPS; i++)
        {
            gainStep(p, st, playerActor);
        }
    }
}

static void forthEorlingas(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    skill_active_eowynForthEorlingas(playerActor);
    if (isEowyn(p) && oddsUnlocked(p))
    {
        state(p)->eorlingasFrames = EORLINGAS_FRAMES;
        state(p)->oddsTimer = 0;
    }
}

void skillExpansion_eowynWrapStockCallbacks(u8 playerIndex, ActiveSkill *rows)
{
    (void)playerIndex;
    rows[ACTIVE_SKILL_EOWYN_ROHAN_SPRINT].callback = rohanSprint;
    rows[ACTIVE_SKILL_EOWYN_FORTH_EORLINGAS].callback = forthEorlingas;
}

SkillExpansionCastResult skillExpansion_eowynDeathSeeker(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    EowynState *st = state(p);
    Actor *foe = nearestFoe(playerActor, DEATH_SEEKER_REACH);
    s32 costPercent = DEATH_SEEKER_HP_PERCENT -
                      passiveLevel(p, SKILL_EXPANSION_PASSIVE_SEEKING_DEATH);
    s32 cost;
    s32 damage;

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    if (costPercent < DEATH_SEEKER_MIN_HP_PERCENT)
    {
        costPercent = DEATH_SEEKER_MIN_HP_PERCENT;
    }
    cost = percentOf(PLAYER(p).currentHp, costPercent);
    if (cost >= PLAYER(p).currentHp)
    {
        cost = PLAYER(p).currentHp - 1;
    }
    if (cost > 0)
    {
        // The price is paid straight to the stock HP change, past every damage guard.
        skillExpansion_original_player_addHp(p, -cost, FALSE);
    }
    damage = DEATH_SEEKER_BASE_DAMAGE + DEATH_SEEKER_DAMAGE_PER_LEVEL * level;
    damage += percentOf(damage, DEATH_SEEKER_PERCENT_PER_TIER * st->tier);
    skillExpansion_dealDamage(playerActor, foe, damage > 0xffff ? 0xffff : damage,
                              HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);
    hackLib_fxBurst(foe, FX_SHAPE_TENDRIL_STAR, FX_COLOR_BLOOD, FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_RISING_STREAKS, FX_COLOR_BLOOD, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_CRIT_HIT);
    st->oddsTimer = 0;
    skillExpansion_feedback(p, "Death! Ride to death!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_eowynComeAndFaceMe(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EowynState *st = state(p);
    Actor *foe = NULL;
    u32 pulled = 0;

    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        s32 distance;

        if (!skillExpansion_isFoe(foe) ||
            distanceSquared(playerActor, foe) > FACE_RADIUS * FACE_RADIUS)
        {
            continue;
        }
        distance = distanceSquared(playerActor, foe);
        if (distance > FACE_CLOSEST * FACE_CLOSEST)
        {
            s32 room = squareRoot(distance) - FACE_CLOSEST;

            knockBack(playerActor, foe, -(room < FACE_PULL ? room : FACE_PULL));
        }
        hackLib_applyStun(foe, FACE_STUN_FRAMES);
        hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
        pulled++;
    }
    if (pulled == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    st->faceFoes = pulled > 0xff ? 0xff : pulled;
    st->faceFrames = FACE_FRAMES;
    st->oddsTimer = 0;
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_GOLD, FX_LAYER_BEHIND);
    hackLib_fxTimed(playerActor, FX_SHAPE_DIAMOND_RING, FX_COLOR_GOLD, FACE_FRAMES,
                    FX_LAYER_BEHIND);
    sfx_play(SFX_FORTH_EORLINGAS);
    skillExpansion_feedback(p, "Come and face me!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_eowynLance(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    EowynState *st = state(p);
    Vector2Int facing = facingOf(playerActor);
    bool gallop = gallopLearned(p) && st->gallop != 0;
    bool pierce = gallop && st->gallop >= GALLOP_MAX;
    s32 damage = LANCE_BASE_DAMAGE + LANCE_DAMAGE_PER_LEVEL * level;
    Actor *foe = NULL;
    Actor *first = NULL;
    s32 best = LANCE_LENGTH + 1;
    u32 hits = 0;

    damage += percentOf(damage, gallop ? LANCE_PERCENT_PER_STEP * st->gallop
                                       : LANCE_PERCENT_PER_TIER * st->tier);
    if (damage > 0xffff)
    {
        damage = 0xffff;
    }
    hackLib_fxAt(playerActor, &playerActor->coords, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE);
    slide(playerActor, facing.x, facing.y, LANCE_LUNGE);
    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        s32 along;

        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        along = alongLine(playerActor, foe, LANCE_LENGTH);
        if (along < 0)
        {
            continue;
        }
        if (pierce)
        {
            HackLibDamageResult result = skillExpansion_dealDamage(
                playerActor, foe, damage, HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);

            if (result.hit && !result.killed)
            {
                knockBack(playerActor, foe, LANCE_KNOCKBACK);
            }
            hackLib_fxBurst(foe, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
            hits++;
        }
        else if (along < best)
        {
            best = along;
            first = foe;
        }
    }
    if (first != NULL)
    {
        HackLibDamageResult result = skillExpansion_dealDamage(
            playerActor, first, damage, HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);

        if (result.hit && !result.killed)
        {
            knockBack(playerActor, first, LANCE_KNOCKBACK);
        }
        hackLib_fxBurst(first, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
        hits++;
    }
    if (gallop && !gallopLocked(p, st))
    {
        st->gallop = 0;
        st->gallopProgress = 0;
        st->barFlashFrames = BAR_FLASH_FRAMES;
    }
    sfx_play(hits != 0 ? SFX_COMBAT_CRIT_HIT : SFX_ROHAN_SPRINT);
    skillExpansion_feedback(p, pierce ? "Through them all!" : "For the Mark!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_eowynStandOverTheFallen(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EowynState *st = state(p);

    st->standLevel = castLevel(p);
    st->standRow = PLAYER(p).castingActiveSkill;
    st->standFrames = STAND_BASE_FRAMES + STAND_FRAMES_PER_LEVEL * st->standLevel;
    st->standPos = playerActor->coords;
    st->reflectDamage = 0;
    hackLib_fxTimed(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, st->standFrames,
                    FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_GROUND_POOL, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    sfx_play(SFX_SHIELD_BASH);
    skillExpansion_feedback(p, "Begone, foul dwimmerlaik!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_eowynHopeRekindled(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    EowynState *st = state(p);
    u32 odds = st->odds > HOPE_MAX_ODDS ? HOPE_MAX_ODDS : st->odds;
    s32 heal = maxHp(p) * (HOPE_BASE_PER_MILLE + HOPE_PER_MILLE_PER_LEVEL * level) * (s32)odds /
               1000;

    player_addHp(p, heal > 0 ? heal : 1, FALSE);
    castFx(playerActor, FX_SHAPE_RISING_STREAKS, odds >= 9 ? FX_COLOR_WHITE : FX_COLOR_GOLD,
           FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GREEN, FX_LAYER_BEHIND);
    sfx_play(SFX_HERB_PICKUP);
    skillExpansion_feedback(p, odds >= 9 ? "Hope is rekindled!" : "A little hope.");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_eowynNoMan(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    EowynState *st = state(p);
    Actor *foe = NULL;

    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) &&
            distanceSquared(playerActor, foe) <= NO_MAN_RADIUS * NO_MAN_RADIUS)
        {
            hackLib_applyStun(foe, NO_MAN_STUN_FRAMES);
        }
    }
    st->noManFrames = NO_MAN_BASE_FRAMES + NO_MAN_FRAMES_PER_LEVEL * castLevel(p);
    st->oddsTimer = 0;
    if (gallopLearned(p))
    {
        st->gallop = GALLOP_MAX;
    }
    playerActor->as.combat.fearDuration = playerActor->as.combat.fearDuration > 1
                                              ? 1
                                              : playerActor->as.combat.fearDuration;
    castFx(playerActor, FX_SHAPE_SHOCKWAVE, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
    hackLib_fxTimed(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_WHITE, st->noManFrames,
                    FX_LAYER_FRONT);
    sfx_play(SFX_FORTH_EORLINGAS);
    skillExpansion_feedback(p, "I am no man!");
    return SKILL_CAST_SUCCESS;
}

bool32 skillExpansion_canCastEowynDeathSeeker(u8 playerIndex)
{
    return PLAYER(playerIndex).currentHp * 100 > maxHp(playerIndex) * DEATH_SEEKER_FLOOR_PERCENT;
}

bool32 skillExpansion_canCastEowynStand(u8 playerIndex)
{
    return state(playerIndex)->standFrames == 0;
}

bool32 skillExpansion_canCastEowynNoMan(u8 playerIndex)
{
    EowynState *st = state(playerIndex);

    return oddsUnlocked(playerIndex) && st->noManFrames == 0 && st->tier >= TIER_DESPERATE;
}

static const HackLibHudMeterLook TierLooks[TIER_COUNT] = {
    {HACKLIB_HUD_COLOR_PALE_GOLD, HACKLIB_HUD_COLOR_BROWN, HACKLIB_HUD_COLOR_DARK_GOLD,
     HACKLIB_HUD_COLOR_WHITE},
    {HACKLIB_HUD_COLOR_PALE_GOLD, HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_DARK_GOLD,
     HACKLIB_HUD_COLOR_WHITE},
    {HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_EMBER, HACKLIB_HUD_COLOR_RED,
     HACKLIB_HUD_COLOR_WHITE},
    {HACKLIB_HUD_COLOR_WHITE, HACKLIB_HUD_COLOR_WHITE, HACKLIB_HUD_COLOR_PALE_GOLD,
     HACKLIB_HUD_COLOR_WHITE},
};
static const HackLibHudMeterLook GallopLook = {HACKLIB_HUD_COLOR_LIGHT_GREEN, HACKLIB_HUD_COLOR_GREEN,
                                               HACKLIB_HUD_COLOR_DARK_GREEN,
                                               HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook GallopFullLook = {HACKLIB_HUD_COLOR_PALE_GOLD,
                                                   HACKLIB_HUD_COLOR_GOLD,
                                                   HACKLIB_HUD_COLOR_DARK_GOLD,
                                                   HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook FlashLook = {HACKLIB_HUD_COLOR_WHITE, HACKLIB_HUD_COLOR_WHITE,
                                              HACKLIB_HUD_COLOR_PALE_GOLD,
                                              HACKLIB_HUD_COLOR_WHITE};

static const HackLibHudMeter TierMeterWide = {HACKLIB_HUD_GAP_X, HACKLIB_HUD_GAP_Y,
                                              HACKLIB_HUD_GAP_WIDTH, HACKLIB_HUD_FREE_TILE};
static const HackLibHudMeter TierMeterHalf = {HACKLIB_HUD_GAP_X, HACKLIB_HUD_GAP_Y,
                                              HACKLIB_HUD_GAP_WIDTH / 2, HACKLIB_HUD_FREE_TILE};
static const HackLibHudMeter GallopMeter = {
    HACKLIB_HUD_GAP_X + HACKLIB_HUD_GAP_WIDTH / 2, HACKLIB_HUD_GAP_Y, HACKLIB_HUD_GAP_WIDTH / 2,
    HACKLIB_HUD_FREE_TILE + HACKLIB_HUD_GAP_WIDTH / 2};

void skillExpansion_eowynDrawHud(u8 playerIndex)
{
    const HackLibHudMeterLook *looks[GALLOP_MAX];
    EowynState *st;
    bool gallop;
    u32 i;

    if (!isEowyn(playerIndex) || !oddsUnlocked(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    gallop = gallopLearned(playerIndex);
    for (i = 0; i < TIER_COUNT; i++)
    {
        looks[i] = i <= st->tier ? &TierLooks[i] : NULL;
    }
    hackLib_drawHudSegments(gallop ? &TierMeterHalf : &TierMeterWide, looks, TIER_COUNT, 1, 1);
    if (!gallop)
    {
        return;
    }
    for (i = 0; i < GALLOP_MAX; i++)
    {
        looks[i] = st->barFlashFrames != 0    ? &FlashLook
                   : i >= st->gallop          ? NULL
                   : st->gallop >= GALLOP_MAX ? &GallopFullLook
                                              : &GallopLook;
    }
    hackLib_drawHudSegments(&GallopMeter, looks, GALLOP_MAX, 1, 1);
}
