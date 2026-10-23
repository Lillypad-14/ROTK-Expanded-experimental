#include "patches/skill_expansion/skill_expansion_legolas.h"
#include "fx.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "lib/combat_source.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "sfx.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

#define FRAMES_PER_SECOND 30

// Cast visuals step every CAST_FX_TICKS so they read instead of flashing by.
#define CAST_FX_TICKS 3

// 1/sqrt(2) scaled by 256, for diagonal facings
#define DIAGONAL_SCALE 181

#define GLINT_UNLOCK_LEVEL 10

// B held this long without a break starts Overdraw. Full draw is when the previous frame is 9.
#define OVERDRAW_CHARGE_FRAMES 15
#define FULL_DRAW_FRAME 9
#define HOLD_FRAMES_CAP 0xff

// Frames at full draw before the glint, then frames the glint stays open for a True Shot.
#define GLINT_FRAMES 12
#define GLINT_WINDOW_FRAMES 6
#define DRAW_FRAMES_CAP 0xfe

// Legolas glows toward the streak color while the glint is open, then fades out.
#define LEGOLAS_OBJ_PALETTE (0x21 + HERO_ID_LEGOLAS)
#define GLOW_STRENGTH 12
#define GLOW_FADE_STEP 3

// Stock arrows of a True Shot hit harder for this long after the loose.
#define TRUE_ARROW_FRAMES 45
#define TRUE_ARROW_PERCENT 25

#define STREAK_MAX 5

// Nocked arrow rows, in page 2 order
#define ROW_PIERCING (SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 0)
#define ROW_SURF (SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 1)
#define ROW_PINNING (SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 2)
#define ROW_GIFT (SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 3)
#define ROW_VOLLEY (SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 4)
#define ROW_ONLY_ONE (SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 5)

#define LINE_LENGTH 240
#define LINE_HALF_WIDTH 16
#define LINE_FX_STEP 40

// Piercing Shot
#define PIERCING_BASE_DAMAGE 14
#define PIERCING_DAMAGE_PER_LEVEL 6
#define PIERCING_TRUE_PERCENT 150

// Shield-Surf
#define SURF_DISTANCE 96
#define SURF_STEP 4
#define SURF_FRAMES FRAMES_PER_SECOND

// Pinning Shot
#define PIN_BASE_FRAMES 45
#define PIN_FRAMES_PER_LEVEL 9
#define PIN_SPREAD_RADIUS 40

// Gift of the Galadhrim
#define GIFT_BASE_SPIRIT 4
#define GIFT_SPIRIT_PER_LEVEL 2

// Volley of Helm's Deep
#define VOLLEY_REACH 96
#define VOLLEY_RADIUS 64
#define VOLLEY_DELAY_FRAMES FRAMES_PER_SECOND
#define VOLLEY_WAVE_FRAMES 10
#define VOLLEY_WAVES 3
#define VOLLEY_TRUE_WAVES 5
#define VOLLEY_BASE_DAMAGE 6
#define VOLLEY_DAMAGE_PER_LEVEL 3

// That Still Only Counts as One!
#define ONLY_ONE_BASE_DAMAGE 30
#define ONLY_ONE_DAMAGE_PER_LEVEL 10
#define ONLY_ONE_PERCENT_PER_STREAK 50

// Passives
#define SWIFT_NOCK_PERCENT_PER_LEVEL 8
#define CLOAK_FRAMES (2 * FRAMES_PER_SECOND)
#define CLOAK_PERCENT_PER_LEVEL 6
#define STEADY_PERCENT_PER_LEVEL 5
#define FLETCHER_PERCENT_PER_LEVEL 10
#define POINT_BLANK_RADIUS 48
#define POINT_BLANK_PUSH 24
#define POINT_BLANK_BASE_FRAMES (FRAMES_PER_SECOND / 2)
#define POINT_BLANK_FRAMES_PER_LEVEL 3

// Spirit cost of each page 2 row before Swift Nock
static const u16 NockCosts[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {20, 15, 20, 15, 35, 50};

static const char *const NockMessages[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    "Piercing Shot nocked", "Shield-Surf nocked", "Pinning Shot nocked",
    "Gift nocked",          "Volley nocked",      "Only Counts as One nocked",
};

typedef struct LegolasState
{
    // Volley target in pixels
    s16 volleyX;
    s16 volleyY;

    // Spirit paid for the nocked arrow
    u16 nockPaid;

    // page 2 row of the nocked arrow, 0 when none is nocked
    u8 nockRow;

    // skill level of the nocked arrow at the nock
    u8 nockLevel;

    // frames B has been held at full draw, 0 when not drawing
    u8 drawFrames;

    // TRUE while B was held on the previous input read
    bool bHeld;

    // TRUE when a loose waits for the tick, and whether it was a True Shot
    bool loosePending;
    bool looseTrue;

    // back-to-back True Shots, 0..STREAK_MAX
    u8 streak;

    // TRUE once "Unerring!" showed for this streak
    bool unerringShown;

    // frames the stock arrows of the last True Shot keep their bonus
    u8 trueArrowFrames;

    // streak when that True Shot loosed
    u8 trueArrowStreak;

    // Elven Cloak frames left
    u8 cloakFrames;

    // frames Legolas stays untouchable after a True Shield-Surf
    u8 surfFrames;

    // Volley delay, then frames to the next wave
    u8 volleyFrames;
    u8 volleyWaves;
    u8 volleyLevel;

    // TRUE while the Volley waves hit at half
    bool volleyHalf;

    // frames B has been held without a break, capped at HOLD_FRAMES_CAP
    u8 holdFrames;

    // glow strength loaded into the sprite, 0..FX_TINT_FULL
    u8 glowStrength;

    // TRUE once the release hint showed in this area
    bool hintShown;
} LegolasState;

_Static_assert(sizeof(LegolasState) <= sizeof(skillExpansion_heroState[0]),
               "LegolasState exceeds the hero state");

extern u8 skillExpansion_treePage;

static inline LegolasState *state(u8 playerIndex)
{
    return (LegolasState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_LEGOLAS,
                                                    PLAYER(playerIndex).heroId);
}

static const Vector2Int FacingVectors[8] = {
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
};

static inline bool isLegolas(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_LEGOLAS;
}

static inline bool glintUnlocked(u8 playerIndex)
{
    return PLAYER(playerIndex).level + 1 >= GLINT_UNLOCK_LEVEL;
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline s32 pixel(s32 coordinate)
{
    return coordinate >> FP16_16_SHIFT;
}

static Actor *castFx(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(playerActor, shape, color, layer), CAST_FX_TICKS);
}

static void fxAtPixel(Actor *origin, s32 x, s32 y, u8 shape, HackLibFxColor color)
{
    Vector2Fp16 position = {x << FP16_16_SHIFT, y << FP16_16_SHIFT};

    hackLib_fxPace(hackLib_fxAt(origin, &position, shape, color), CAST_FX_TICKS);
}

static Vector2Int facingOf(const Actor *actor)
{
    return FacingVectors[actor->facing < 8 ? actor->facing : DIRECTION8_SOUTH];
}

/** The point `distance` pixels ahead of the actor, in pixels. */
static void pointAhead(const Actor *actor, s32 distance, s32 *x, s32 *y)
{
    Vector2Int facing = facingOf(actor);

    if (facing.x != 0 && facing.y != 0)
    {
        distance = distance * DIAGONAL_SCALE / 256;
    }
    *x = pixel(actor->coords.x) + facing.x * distance;
    *y = pixel(actor->coords.y) + facing.y * distance;
}

/** Distance of the foe along the actor's facing line, or -1 when it is off the line. */
static s32 alongLine(const Actor *actor, const Actor *foe)
{
    Vector2Int facing = facingOf(actor);
    s32 scale = facing.x != 0 && facing.y != 0 ? DIAGONAL_SCALE : 256;
    s32 dx = pixel(foe->coords.x) - pixel(actor->coords.x);
    s32 dy = pixel(foe->coords.y) - pixel(actor->coords.y);
    s32 along = (dx * facing.x + dy * facing.y) * scale / 256;
    s32 across = ABS(dx * facing.y - dy * facing.x) * scale / 256;

    if (along < 0 || along > LINE_LENGTH || across > LINE_HALF_WIDTH)
    {
        return -1;
    }
    return along;
}

static void lineFx(Actor *actor, u8 shape, HackLibFxColor color)
{
    s32 distance;

    for (distance = LINE_FX_STEP; distance <= LINE_LENGTH; distance += LINE_FX_STEP)
    {
        s32 x;
        s32 y;

        pointAhead(actor, distance, &x, &y);
        fxAtPixel(actor, x, y, shape, color);
    }
}

/** The nearest foe on the actor's facing line, or NULL. */
static Actor *firstOnLine(Actor *actor)
{
    Actor *foe = NULL;
    Actor *first = NULL;
    s32 best = LINE_LENGTH + 1;

    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        s32 along;

        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        along = alongLine(actor, foe);
        if (along >= 0 && along < best)
        {
            best = along;
            first = foe;
        }
    }
    return first;
}

/** Move the foe up to `distance` pixels straight away from (x, y), stopping at walls. */
static void pushFoe(Actor *foe, s32 x, s32 y, s32 distance)
{
    s32 dx = pixel(foe->coords.x) - x;
    s32 dy = pixel(foe->coords.y) - y;
    s32 length = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
    Vector2Fp16 landing = foe->coords;
    s32 step;

    if (length == 0)
    {
        return;
    }
    for (step = 1; step <= distance; step++)
    {
        Vector2Fp16 next = {foe->coords.x + dx * step * FP16_16_ONE / length,
                            foe->coords.y + dy * step * FP16_16_ONE / length};

        if (!hackLib_canLandAt(foe, &next))
        {
            break;
        }
        landing = next;
    }
    foe->renderCoords.x += landing.x - foe->coords.x;
    foe->renderCoords.y += landing.y - foe->coords.y;
    foe->coords = landing;
    foe->velocity.x = foe->velocity.y = 0;
}

#define GLOW_COLOR(r, g, b) ((r) | (g) << 5 | (b) << 10)

static const u16 GlowColors[STREAK_MAX + 1] = {
    GLOW_COLOR(31, 31, 31), GLOW_COLOR(16, 31, 6), GLOW_COLOR(16, 31, 6),
    GLOW_COLOR(31, 28, 0),  GLOW_COLOR(31, 28, 0), GLOW_COLOR(8, 22, 31),
};

static const char *const TrueShotMessages[STREAK_MAX] = {
    "True Shot!", "True Shot x2", "True Shot x3", "True Shot x4", "True Shot x5",
};

// TRUE once the player has loosed a True Shot since power on.
static bool trueShotLearned;

static HackLibFxColor streakColor(u8 streak)
{
    if (streak >= STREAK_MAX)
    {
        return FX_COLOR_BLUE;
    }
    if (streak >= 3)
    {
        return FX_COLOR_GOLD;
    }
    return streak != 0 ? FX_COLOR_GREEN : FX_COLOR_WHITE;
}

static bool hasBow(u8 playerIndex)
{
    Item mainHand = PLAYER(playerIndex).inventory.slots.mainhand;

    return mainHand.d.itemType != ITEM_TYPE_EMPTY && ITEM_BASE_METADATA(mainHand).flags.d.familyBow;
}

static u8 rowLevel(u8 playerIndex, u8 row)
{
    u8 level = skillExpansion_effectiveActiveLevel(playerIndex, row);

    return level != 0 ? level : 1;
}

/** Hit every foe on the line. Percent scales the damage. */
static void piercingShot(Actor *actor, u8 level, s32 percent)
{
    s32 damage = (PIERCING_BASE_DAMAGE + PIERCING_DAMAGE_PER_LEVEL * level) * percent / 100;
    Actor *foe = NULL;

    lineFx(actor, FX_SHAPE_SPARK, FX_COLOR_WHITE);
    sfx_play(SFX_COMBAT_HIT_MISSILE);
    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || alongLine(actor, foe) < 0)
        {
            continue;
        }
        if (skillExpansion_dealDamage(actor, foe, damage < 1 ? 1 : damage, HACKLIB_DAMAGE_ABILITY,
                                      ROW_PIERCING)
                .hit)
        {
            hackLib_fxBurst(foe, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
    }
}

/** Slide backwards from the facing, stopping at walls. */
static void shieldSurf(u8 playerIndex, Actor *actor, LegolasState *st, bool full)
{
    Vector2Int facing = facingOf(actor);
    s32 reach = full ? SURF_DISTANCE : SURF_DISTANCE / 2;
    s32 step = facing.x != 0 && facing.y != 0 ? SURF_STEP * DIAGONAL_SCALE / 256 : SURF_STEP;
    Vector2Fp16 end = actor->coords;
    s32 travelled;

    for (travelled = SURF_STEP; travelled <= reach; travelled += SURF_STEP)
    {
        Vector2Fp16 next = {end.x - (facing.x * step << FP16_16_SHIFT),
                            end.y - (facing.y * step << FP16_16_SHIFT)};

        if (!hackLib_canLandAt(actor, &next))
        {
            break;
        }
        end = next;
    }
    fxAtPixel(actor, pixel(actor->coords.x), pixel(actor->coords.y), FX_SHAPE_SMOKE_SMALL,
              FX_COLOR_WHITE);
    actor->renderCoords.x += end.x - actor->coords.x;
    actor->renderCoords.y += end.y - actor->coords.y;
    actor->coords = end;
    actor->velocity.x = actor->velocity.y = 0;
    castFx(actor, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_GREEN, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_PLAYER_DODGE);
    if (full)
    {
        st->surfFrames = SURF_FRAMES;
        actor->flags.p |= ACTOR_FLAG_23;
        skillExpansion_feedback(playerIndex, "Untouchable");
    }
}

/** Stun the first foe on the line, and with `spread` every foe near it. */
static void pinningShot(Actor *actor, u8 level, bool full, bool spread)
{
    Actor *target = firstOnLine(actor);
    s32 frames = PIN_BASE_FRAMES + PIN_FRAMES_PER_LEVEL * level;

    lineFx(actor, FX_SHAPE_SPARK, FX_COLOR_GREEN);
    if (target == NULL)
    {
        return;
    }
    if (!full)
    {
        frames /= 2;
    }
    hackLib_applyStun(target, frames);
    hackLib_fxBurst(target, FX_SHAPE_DIAMOND_RING, FX_COLOR_GREEN, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    if (spread)
    {
        Actor *foe = NULL;

        while ((foe = hackLib_nextTargetAt(&target->coords, actor, PIN_SPREAD_RADIUS, foe)) !=
               NULL)
        {
            if (foe != target && skillExpansion_isFoe(foe))
            {
                hackLib_applyStun(foe, frames);
                hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
            }
        }
    }
}

/** Spirit for every foe on the line. */
static void giftOfTheGaladhrim(u8 playerIndex, Actor *actor, u8 level, bool full)
{
    s32 perFoe = GIFT_BASE_SPIRIT + GIFT_SPIRIT_PER_LEVEL * level;
    s32 spirit = 0;
    Actor *foe = NULL;

    lineFx(actor, FX_SHAPE_SPARK, FX_COLOR_GREEN);
    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && alongLine(actor, foe) >= 0)
        {
            spirit += full ? perFoe : perFoe / 2;
            hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
        }
    }
    if (spirit != 0)
    {
        player_addSpirit(playerIndex, spirit);
        castFx(actor, FX_SHAPE_RING_SPARKLES, FX_COLOR_GREEN, FX_LAYER_FRONT);
        sfx_play(SFX_HERB_PICKUP);
    }
}

static void startVolley(Actor *actor, LegolasState *st, u8 level, bool full)
{
    s32 x;
    s32 y;

    pointAhead(actor, VOLLEY_REACH, &x, &y);
    st->volleyX = x;
    st->volleyY = y;
    st->volleyFrames = VOLLEY_DELAY_FRAMES;
    st->volleyWaves = full ? VOLLEY_TRUE_WAVES : VOLLEY_WAVES;
    st->volleyLevel = level;
    st->volleyHalf = !full;
    castFx(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GREEN, FX_LAYER_FRONT);
    fxAtPixel(actor, x, y, FX_SHAPE_OVAL_RING, FX_COLOR_WHITE);
}

static void tickVolley(Actor *actor, LegolasState *st)
{
    Vector2Fp16 center;
    Actor *foe = NULL;
    s32 damage;

    if (st->volleyWaves == 0 || --st->volleyFrames != 0)
    {
        return;
    }
    center.x = st->volleyX << FP16_16_SHIFT;
    center.y = st->volleyY << FP16_16_SHIFT;
    damage = VOLLEY_BASE_DAMAGE + VOLLEY_DAMAGE_PER_LEVEL * st->volleyLevel;
    if (st->volleyHalf)
    {
        damage /= 2;
    }
    fxAtPixel(actor, st->volleyX, st->volleyY, FX_SHAPE_STREAK_RAIN, FX_COLOR_WHITE);
    sfx_play(SFX_COMBAT_HIT_MISSILE);
    while ((foe = hackLib_nextTargetAt(&center, actor, VOLLEY_RADIUS, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) &&
            skillExpansion_dealDamage(actor, foe, damage < 1 ? 1 : damage, HACKLIB_DAMAGE_ABILITY,
                                      ROW_VOLLEY)
                .hit)
        {
            hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
        }
    }
    st->volleyFrames = --st->volleyWaves != 0 ? VOLLEY_WAVE_FRAMES : 0;
}

/** One huge arrow at the first foe on the line, scaled by `streak` and spending it. */
static void onlyCountsAsOne(Actor *actor, LegolasState *st, u8 level, u8 streak, bool full)
{
    Actor *target = firstOnLine(actor);
    s32 damage = (ONLY_ONE_BASE_DAMAGE + ONLY_ONE_DAMAGE_PER_LEVEL * level) *
                 (100 + ONLY_ONE_PERCENT_PER_STREAK * streak) / 100;

    if (!full)
    {
        damage /= 2;
    }
    st->streak = 0;
    st->unerringShown = FALSE;
    lineFx(actor, FX_SHAPE_LIGHTNING, FX_COLOR_WHITE);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    if (target == NULL)
    {
        return;
    }
    if (skillExpansion_dealDamage(actor, target, damage, HACKLIB_DAMAGE_ABILITY, ROW_ONLY_ONE).hit)
    {
        hackLib_fxPace(hackLib_fxBurst(target, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD,
                                       FX_LAYER_FRONT),
                       CAST_FX_TICKS);
    }
}

/** Fire the nocked arrow of `row`. */
static void fireArrow(u8 playerIndex, Actor *actor, LegolasState *st, u8 row, u8 level, bool full,
                      bool isTrue, u8 streakBefore)
{
    switch (row)
    {
    case ROW_PIERCING:
        piercingShot(actor, level, isTrue && full ? PIERCING_TRUE_PERCENT : full ? 100 : 50);
        break;
    case ROW_SURF:
        shieldSurf(playerIndex, actor, st, full);
        break;
    case ROW_PINNING:
        pinningShot(actor, level, full, isTrue);
        break;
    case ROW_GIFT:
        giftOfTheGaladhrim(playerIndex, actor, level, full);
        break;
    case ROW_VOLLEY:
        startVolley(actor, st, level, full);
        break;
    case ROW_ONLY_ONE:
        onlyCountsAsOne(actor, st, level, isTrue ? streakBefore + 1 : streakBefore, full);
        break;
    }
}

/** Knock back and stun foes close to Legolas, for Point Blank. */
static void pointBlank(Actor *actor, s32 level)
{
    s32 x = pixel(actor->coords.x);
    s32 y = pixel(actor->coords.y);
    Actor *foe = NULL;

    castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    while ((foe = hackLib_nextTargetAt(&actor->coords, actor, POINT_BLANK_RADIUS, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            pushFoe(foe, x, y, POINT_BLANK_PUSH);
            hackLib_applyStun(foe,
                              POINT_BLANK_BASE_FRAMES + POINT_BLANK_FRAMES_PER_LEVEL * level);
        }
    }
}

static void loose(u8 playerIndex, Actor *actor, LegolasState *st, bool isTrue)
{
    u8 streakBefore = st->streak;
    u8 row = st->nockRow;

    if (isTrue)
    {
        s32 blank = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_POINT_BLANK);

        if (st->streak < STREAK_MAX)
        {
            st->streak++;
        }
        st->trueArrowFrames = TRUE_ARROW_FRAMES;
        st->trueArrowStreak = st->streak;
        trueShotLearned = TRUE;
        skillExpansion_feedback(playerIndex, TrueShotMessages[st->streak - 1]);
        castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        sfx_play(SFX_COMBAT_CRIT_HIT);
        if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ELVEN_CLOAK) != 0)
        {
            st->cloakFrames = CLOAK_FRAMES;
        }
        if (blank != 0)
        {
            pointBlank(actor, blank);
        }
        if (st->streak == STREAK_MAX)
        {
            if (!st->unerringShown)
            {
                st->unerringShown = TRUE;
                skillExpansion_feedback(playerIndex, "Unerring!");
            }
            if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNERRING) != 0)
            {
                piercingShot(actor, rowLevel(playerIndex, ROW_PIERCING), 50);
            }
        }
    }
    else
    {
        st->streak = 0;
        st->unerringShown = FALSE;
    }
    if (row == 0)
    {
        return;
    }
    st->nockRow = 0;
    fireArrow(playerIndex, actor, st, row, st->nockLevel, isTrue, isTrue, streakBefore);
    if (!isTrue)
    {
        return;
    }
    if (row == ROW_GIFT)
    {
        player_addSpirit(playerIndex, st->nockPaid);
    }
    else
    {
        player_addSpirit(playerIndex, st->nockPaid * FLETCHER_PERCENT_PER_LEVEL *
                                          passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FLETCHER) /
                                          100);
    }
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_TWIN_NOCK) != 0 && row != ROW_ONLY_ONE)
    {
        fireArrow(playerIndex, actor, st, row, st->nockLevel, FALSE, FALSE, 0);
    }
}

/** Page 2 costs with Swift Nock. */
static void writeCosts(u8 playerIndex)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    s32 percent = 100 - SWIFT_NOCK_PERCENT_PER_LEVEL *
                            passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SWIFT_NOCK);
    u32 i;

    // The skill tree swaps the pages' rows while it shows page 2.
    if (skillExpansion_treePage != 0)
    {
        return;
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        if (sx->activeDefs[i] != NULL)
        {
            sx->activeSkills[SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i].spiritCost =
                NockCosts[i] * percent / 100;
        }
    }
}

static void loadGlow(Actor *actor, u8 streak, u8 strength)
{
    hackLib_fxTintSlot(actor->oam.paletteNumber, ObjPalettes[LEGOLAS_OBJ_PALETTE], 1, 15,
                       GlowColors[streak], strength);
}

static s32 glintEnd(u8 playerIndex)
{
    return GLINT_FRAMES + GLINT_WINDOW_FRAMES +
           passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ELF_SIGHT);
}

/** Snap the glow on while the glint is open and fade it out after. */
static void showGlow(u8 playerIndex, Actor *actor, LegolasState *st)
{
    bool open = st->drawFrames >= GLINT_FRAMES && st->drawFrames < glintEnd(playerIndex);

    if (open)
    {
        st->glowStrength = GLOW_STRENGTH;
    }
    else if (st->glowStrength == 0)
    {
        return;
    }
    else
    {
        st->glowStrength = st->glowStrength > GLOW_FADE_STEP ? st->glowStrength - GLOW_FADE_STEP : 0;
    }
    loadGlow(actor, st->streak, st->glowStrength);
}

static void clearState(Actor *actor, LegolasState *st)
{
    if (actor != NULL && st->surfFrames != 0)
    {
        actor->flags.p &= ~ACTOR_FLAG_23;
    }
    if (actor != NULL && st->glowStrength != 0)
    {
        loadGlow(actor, 0, 0);
    }
    memory_memset(st, 0, sizeof(LegolasState));
}

void skillExpansion_legolasReset(u8 playerIndex)
{
    if (!isLegolas(playerIndex))
    {
        return;
    }
    clearState(NULL, state(playerIndex));
}

void skillExpansion_legolasCleanup(u8 playerIndex, Actor *actor)
{
    if (!isLegolas(playerIndex))
    {
        return;
    }
    clearState(actor, state(playerIndex));
}

void skillExpansion_legolasInput(u8 playerIndex)
{
    LegolasState *st;
    Actor *actor = PLAYER(playerIndex).ownerActor;
    bool held;
    bool fullDraw;

    if (!isLegolas(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    held = (PLAYER_KEYS_CURRENT(playerIndex) & B_BUTTON) != 0;
    if (!held)
    {
        st->holdFrames = 0;
    }
    else if (st->holdFrames < HOLD_FRAMES_CAP)
    {
        st->holdFrames++;
    }
    if (!glintUnlocked(playerIndex))
    {
        st->bHeld = held;
        return;
    }
    fullDraw = actor->actionState == ACTOR_STATE_LEGOLAS_OVERDRAW &&
               actor->previousAnimationFrame == FULL_DRAW_FRAME;
    if (fullDraw && st->bHeld && !held)
    {
        bool early = st->drawFrames < GLINT_FRAMES;

        st->loosePending = TRUE;
        st->looseTrue = !early && st->drawFrames < glintEnd(playerIndex);
        st->drawFrames = 0;
        // Misses are called out while learning, or when they cost a streak or a nocked arrow.
        if (!st->looseTrue && (!trueShotLearned || st->streak != 0 || st->nockRow != 0))
        {
            skillExpansion_feedback(playerIndex, early ? "Too early" : "Too late");
        }
    }
    else if (fullDraw && held)
    {
        if (st->drawFrames < DRAW_FRAMES_CAP)
        {
            st->drawFrames++;
        }
        if (st->drawFrames == 1)
        {
            castFx(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
            if (!trueShotLearned && !st->hintShown)
            {
                st->hintShown = TRUE;
                skillExpansion_feedback(playerIndex, "Release B on the flash");
            }
        }
        else if (st->drawFrames == GLINT_FRAMES)
        {
            hackLib_fxBurst(actor, FX_SHAPE_TENDRIL_STAR, streakColor(st->streak), FX_LAYER_FRONT);
            sfx_play(SFX_GEM_PICKUP);
        }
    }
    else
    {
        st->drawFrames = 0;
    }
    st->bHeld = held;
}

void skillExpansion_legolasTick(u8 playerIndex, Actor *actor)
{
    LegolasState *st;

    if (!isLegolas(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (PLAYER(playerIndex).currentHp <= 0)
    {
        if (st->nockRow != 0 || st->streak != 0 || st->volleyWaves != 0)
        {
            clearState(actor, st);
        }
        return;
    }
    if (st->loosePending)
    {
        st->loosePending = FALSE;
        loose(playerIndex, actor, st, st->looseTrue);
    }
    if (st->trueArrowFrames != 0)
    {
        st->trueArrowFrames--;
    }
    if (st->cloakFrames != 0)
    {
        st->cloakFrames--;
    }
    if (st->surfFrames != 0 && --st->surfFrames == 0)
    {
        actor->flags.p &= ~ACTOR_FLAG_23;
    }
    showGlow(playerIndex, actor, st);
    tickVolley(actor, st);
    writeCosts(playerIndex);
}

s32 skillExpansion_legolasDamage(u8 playerIndex, Actor *victim, s32 damage, bool melee)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    s32 percent;

    if (!isLegolas(playerIndex) || victim == NULL || melee || source == NULL ||
        source->kind != HACKLIB_DAMAGE_PROJECTILE)
    {
        return damage;
    }
    LegolasState *st = state(playerIndex);
    if (st->trueArrowFrames == 0)
    {
        return damage;
    }
    percent = TRUE_ARROW_PERCENT + STEADY_PERCENT_PER_LEVEL * st->trueArrowStreak *
                                       passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STEADY_HAND);
    return damage + damage * percent / 100;
}

s32 skillExpansion_legolasIncomingDamage(u8 playerIndex, s32 damage)
{
    LegolasState *st;

    if (!isLegolas(playerIndex) || damage <= 0)
    {
        return damage;
    }
    st = state(playerIndex);
    if (st->surfFrames != 0)
    {
        return 0;
    }
    if (st->cloakFrames != 0)
    {
        damage -= damage * CLOAK_PERCENT_PER_LEVEL *
                  passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ELVEN_CLOAK) / 100;
        if (damage < 1)
        {
            damage = 1;
        }
    }
    return damage;
}

bool skillExpansion_legolasFlinch(u8 playerIndex, bool flinchOnDamage)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;

    if (isLegolas(playerIndex) && actor != NULL &&
        actor->actionState == ACTOR_STATE_LEGOLAS_OVERDRAW &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNSHAKEN) != 0)
    {
        return FALSE;
    }
    return flinchOnDamage;
}

u8 skillExpansion_legolasNockedRow(void)
{
    u8 p = ACTIVE_PLAYER_INDEX;

    return isLegolas(p) ? state(p)->nockRow : 0;
}

const char *skillExpansion_legolasRejectMessage(u8 playerIndex, u8 row, const char *message)
{
    if (isLegolas(playerIndex) && row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW &&
        row < SKILL_EXPANSION_ACTIVE_ROW_COUNT && !hasBow(playerIndex))
    {
        return "Needs a bow";
    }
    return message;
}

s32 skillExpansion_legolasOverdrawCharge(s32 playerIndex, Actor *actor)
{
    Player *player = &PLAYER(playerIndex);

    // Input counts the hold every frame, so taps never add up and a hold through a shot still
    // charges.
    player->bowHoldFrames = 0;
    if (player->heroId == HERO_ID_LEGOLAS && (PLAYER_KEYS_CURRENT(playerIndex) & B_BUTTON))
    {
        LegolasState *st = state(playerIndex);

        if (st->holdFrames >= OVERDRAW_CHARGE_FRAMES &&
            actor->actionState != ACTOR_STATE_LEGOLAS_OVERDRAW && hasBow(playerIndex))
        {
            st->holdFrames = 0;
            actor->velocity.x = 0;
            actor->velocity.y = 0;
            actor->flags.p |= ACTOR_FLAG_ANIMATION_PLAYING;
            actor->as.combat.requestedPose = 0;
            actor->as.combat.head.player.animationBank =
                (void *)HeroMeleeAnimationBanks[player->heroId];
            return ACTOR_STATE_LEGOLAS_OVERDRAW;
        }
    }
    return -1;
}

SkillExpansionCastResult skillExpansion_legolasNock(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    LegolasState *st = state(p);
    u8 row = PLAYER(p).castingActiveSkill;

    if (row < SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    // One arrow on the string: the old one goes back to the quiver.
    if (st->nockRow != 0)
    {
        player_addSpirit(p, st->nockPaid);
    }
    st->nockRow = row;
    st->nockLevel = skillExpansion_effectiveActiveLevel(p, row);
    st->nockPaid = skillExpansion_runtime[p].paidSpirit;
    castFx(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
    sfx_play(SFX_SKILL_TREE_BUY_ACTIVE);
    skillExpansion_feedback(p, NockMessages[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW]);
    return SKILL_CAST_SUCCESS;
}

bool32 skillExpansion_canCastLegolasNock(u8 playerIndex)
{
    return hasBow(playerIndex);
}
