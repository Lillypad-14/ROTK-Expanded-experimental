#include "patches/skill_expansion/skill_expansion_gandalf.h"
#include "fx.h"
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "input.h"
#include "lib/combat_source.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/targeting.h"
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

#define RGB5(r, g, b) (((b) << 10) | ((g) << 5) | (r))

// 1/sqrt(2) scaled by 256, for diagonal facings
#define DIAGONAL_SCALE 181

// Form swap
#define FORM_GREY 0
#define FORM_WHITE 1
#define SWAP_UNLOCK_LEVEL 10
#define SWAP_HOLD_FRAMES 15
#define SWAP_COOLDOWN_FRAMES (3 * FRAMES_PER_SECOND)
#define WHITE_MIN_SPIRIT 20
#define WHITE_LOW_SPIRIT 25
#define WHITE_PAGE1_PERCENT 30
#define RETURN_RADIUS 64
#define RETURN_STUN_FRAMES FRAMES_PER_SECOND
#define SMOKE_HIDE_FRAMES (2 * FRAMES_PER_SECOND)

// Pipe-weed renews this hide every tick, so it lasts one tick past the pipe.
#define PIPE_HIDE_FRAMES 2

// White Spirit drain in tenths of Spirit per second, DRAIN_STEP drain units per Spirit
#define DRAIN_TENTHS 30
#define DRAIN_TENTHS_PER_UNBENDING 4
#define DRAIN_STEP (10 * FRAMES_PER_SECOND)
#define RETURNED_DRAIN_PAUSE_FRAMES (10 * FRAMES_PER_SECOND)

// Gandalf's sprite palette. Colors 3..14 are his robe, hat and beard ramps.
#define GANDALF_OBJ_PALETTE (0x21 + HERO_ID_GANDALF)
#define TINT_REFRESH_FRAMES FRAMES_PER_SECOND
#define WHITE_SPARKLE_FRAMES (2 * FRAMES_PER_SECOND)

// HUD Spirit bar fill colors in BG palette bank 0
#define HUD_SPIRIT_FIRST_COLOR 10
#define HUD_SPIRIT_COLOR_COUNT 3
#define HUD_PULSE_FRAMES 8

// Fireworks (Grey)
#define FIREWORKS_REACH 64
#define FIREWORKS_RADIUS 40
#define FIREWORKS_BASE_DAMAGE 8
#define FIREWORKS_DAMAGE_PER_LEVEL 4
#define FIREWORKS_STUN_FRAMES (FRAMES_PER_SECOND / 2)
#define FIREWORKS_POPS 3
#define FIREWORKS_POP_FRAMES 10
#define FIREWORKS_POP_BASE_DAMAGE 4
#define FIREWORKS_POP_DAMAGE_PER_LEVEL 2

// Flame of Anor (White)
#define ANOR_LENGTH 240
#define ANOR_HALF_WIDTH 16
#define ANOR_FX_STEP 40
#define ANOR_BASE_DAMAGE 20
#define ANOR_DAMAGE_PER_LEVEL 8

// A Wizard Is Never Late (Grey)
#define NEVER_LATE_PULSES 4
#define NEVER_LATE_PULSE_FRAMES FRAMES_PER_SECOND

// Shadowfax (White)
#define SHADOWFAX_REACH 160
#define SHADOWFAX_STEP 4
#define SHADOWFAX_HIT_RADIUS 20
#define SHADOWFAX_PUSH 16
#define SHADOWFAX_BASE_DAMAGE 14
#define SHADOWFAX_DAMAGE_PER_LEVEL 6
#define SHADOWFAX_FX_STEP 32

// Smoke Rings (Grey)
#define SMOKE_REACH 64
#define SMOKE_RADIUS 40
#define SMOKE_FRAMES (4 * FRAMES_PER_SECOND)
#define SMOKE_STUN_FRAMES (FRAMES_PER_SECOND / 2)
#define SMOKE_PERCENT_PER_LEVEL 10
#define ZONE_PULSE_FRAMES FRAMES_PER_SECOND

// Breaking of the Staff (White)
#define BROKEN_RADIUS 80
#define BROKEN_BASE_FRAMES (3 * FRAMES_PER_SECOND)
#define BROKEN_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define BROKEN_TAKEN_PERCENT 25
#define BROKEN_DEALT_PERCENT 50

// Pipe-weed Pause (Grey)
#define PIPE_FRAMES (3 * FRAMES_PER_SECOND)
#define PIPE_TICK_FRAMES (FRAMES_PER_SECOND / 2)
#define PIPE_BASE_SPIRIT 8
#define PIPE_SPIRIT_PER_LEVEL 2

// Healing of Theoden (White)
#define THEODEN_BASE_PERCENT 20
#define THEODEN_PERCENT_PER_LEVEL 4
#define THEODEN_PARTNER_RADIUS 96

// Fly, You Fools! (Grey)
#define FLY_RADIUS 80
#define FLY_PUSH 24
#define FLY_STUN_FRAMES (FRAMES_PER_SECOND / 2)
#define FLY_SPEED_PERCENT 50
#define FLY_SPEED_FRAMES (3 * FRAMES_PER_SECOND)

// Look to My Coming (White)
#define COMING_RADIUS 160
#define COMING_BASE_DAMAGE 16
#define COMING_DAMAGE_PER_LEVEL 6
#define COMING_STUN_FRAMES (2 * FRAMES_PER_SECOND)

// You Shall Not Pass! (Grey)
#define PASS_REACH 24
#define PASS_HOLD_RADIUS 40
#define PASS_FRAMES (3 * FRAMES_PER_SECOND)
#define PASS_PUSH_FRAMES 6
#define PASS_PUSH 8
#define PASS_BURST_RADIUS 96
#define PASS_BASE_DAMAGE 30
#define PASS_DAMAGE_PER_LEVEL 10
#define PASS_BURST_STUN_FRAMES FRAMES_PER_SECOND
#define FALL_FRAMES (3 * FRAMES_PER_SECOND)

// Light of Valinor (White)
#define VALINOR_RADIUS 160
#define VALINOR_BASE_PERCENT 40
#define VALINOR_PERCENT_PER_LEVEL 10
#define VALINOR_STUN_FRAMES FRAMES_PER_SECOND

// Passives
#define WANDERING_BASE_FRAMES (3 * FRAMES_PER_SECOND)
#define WANDERING_FRAMES_PER_LEVEL 12
#define ROBES_PERCENT_PER_LEVEL 5
#define KINDLER_RADIUS_PER_LEVEL 12
#define KINDLER_FRAMES_PER_LEVEL 6
#define OLD_TRICKS_PERCENT_PER_LEVEL 10
#define QUICKENED_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define QUICKENED_PERCENT_PER_LEVEL 10
#define WHITE_RIDER_SPIRIT_PER_LEVEL 3
#define SECRET_FIRE_RADIUS 56
#define SECRET_FIRE_BASE_DAMAGE 10

// Grey half then White half of each page 2 row, in Spirit
static const u16 GreyCosts[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {15, 15, 10, 0, 20, 70};
static const u16 WhiteCosts[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {45, 40, 40, 40, 60, 30};

// Gandalf's sprite palette washed to the White's robes. Skin and outline keep their colors.
static const u16 WhiteColors[16] = {
    RGB5(4, 11, 9),    RGB5(30, 22, 19),  RGB5(26, 17, 14),  RGB5(31, 31, 31),
    RGB5(29, 29, 31),  RGB5(26, 26, 29),  RGB5(21, 21, 26),  RGB5(16, 16, 21),
    RGB5(11, 11, 16),  RGB5(5, 5, 9),     RGB5(29, 29, 31),  RGB5(26, 26, 29),
    RGB5(21, 21, 26),  RGB5(16, 16, 21),  RGB5(11, 11, 16),  RGB5(0, 0, 0),
};

// The Spirit bar fill while White: highlight, mid, shadow
static const u16 WhiteSpiritColors[HUD_SPIRIT_COLOR_COUNT] = {
    RGB5(31, 31, 31),
    RGB5(25, 26, 29),
    RGB5(17, 18, 23),
};

typedef struct GandalfState
{
    // A Wizard Is Never Late mark in pixels
    s16 markX;
    s16 markY;

    // HP and Spirit at the mark
    s16 markHp;
    s16 markSpirit;

    // Smoke Rings cloud center in pixels
    s16 smokeX;
    s16 smokeY;

    // Breaking of the Staff center in pixels
    s16 brokenX;
    s16 brokenY;

    // You Shall Not Pass! line center in pixels
    s16 passX;
    s16 passY;

    // Fireworks burst point in pixels
    s16 popX;
    s16 popY;

    // STAT_SPIRIT_REGEN held back while White
    s16 savedRegen;

    // STAT_SPIRIT_REGEN_PERCENT held back while White
    s16 savedRegenPercent;

    // STAT_SPEED_PERCENT while Fly, You Fools! hastes Gandalf, 0 when not hasted
    s16 hastedSpeed;

    // White drain units, or Wandering Wizard frames while Grey
    u16 drainUnits;

    // frames the White drain waits after You Shall Not Pass!
    u16 drainPauseFrames;

    // stock Spirit bar fill colors, valid while hudSaved
    u16 hudStock[HUD_SPIRIT_COLOR_COUNT];

    // FORM_GREY or FORM_WHITE
    u8 form;

    // frames R has been held, 0xff until it is released after a swap
    u8 holdFrames;

    // TRUE once the hold asks for a swap
    bool swapRequested;

    u8 swapCooldownFrames;

    // White palette blend, 0..FX_TINT_FULL
    u8 tintStrength;

    u8 tintFrames;
    u8 sparkleFrames;
    u8 hudPulseFrames;

    // Fireworks pops left and frames to the next
    u8 popsLeft;
    u8 popFrames;

    // skill level and row of the pending Fireworks pops
    u8 popLevel;
    u8 popRow;

    // A Wizard Is Never Late pulses left and frames to the next
    u8 markPulses;
    u8 markFrames;

    u8 smokeFrames;

    // Smoke Rings level at cast
    u8 smokeLevel;

    u8 brokenFrames;

    // Pipe-weed Pause frames left
    u8 pipeFrames;

    // Pipe-weed Pause level at cast
    u8 pipeLevel;

    // You Shall Not Pass! frames left on the line
    u8 passFrames;

    // You Shall Not Pass! level and row at cast
    u8 passLevel;
    u8 passRow;

    // frames left of the Fall (hidden and unhurt)
    u8 fallFrames;

    // frames Gandalf stays hidden by a swap or his pipe
    u8 hideFrames;

    // Fly, You Fools! haste frames left
    u8 hasteFrames;

    // TRUE once The Fall saved Gandalf in this area
    bool fallUsed;

    // TRUE while the next cast after a swap is discounted by Quickened
    bool quickened;

    // TRUE once "The light dims" showed for this low Spirit
    bool lowWarned;

    // TRUE while hudStock holds the stock Spirit bar colors
    bool hudSaved;

    // TRUE once the swap hint showed in this area
    bool hinted;
} GandalfState;

_Static_assert(sizeof(GandalfState) <= sizeof(skillExpansion_heroState[0]),
               "GandalfState exceeds the hero state");

// TRUE once each player has swapped forms since power on, which ends the swap hint.
static bool swapLearned[2];

static inline GandalfState *state(u8 playerIndex)
{
    return (GandalfState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_GANDALF,
                                                    PLAYER(playerIndex).heroId);
}

static const Vector2Int FacingVectors[8] = {
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
};

static inline bool isGandalf(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_GANDALF &&
           skillExpansion_playerExpanded(playerIndex);
}

static inline bool isWhite(u8 playerIndex)
{
    return state(playerIndex)->form == FORM_WHITE;
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline u8 castLevel(u8 playerIndex)
{
    return skillExpansion_effectiveActiveLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
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

static bool within(const Actor *actor, s32 x, s32 y, s32 radius)
{
    s32 dx = pixel(actor->coords.x) - x;
    s32 dy = pixel(actor->coords.y) - y;

    return dx * dx + dy * dy <= radius * radius;
}

static bool32 isFoeTarget(Actor *target, u32 argument)
{
    (void)argument;
    return skillExpansion_isFoe(target);
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

/** Damage, stun and spark every foe within radius of (x, y). Returns the number of foes hit. */
static u32 hitAt(Actor *actor, s32 x, s32 y, s32 radius, s32 damage, s32 stunFrames, u8 row,
                 HackLibFxColor color)
{
    Vector2Fp16 center = {x << FP16_16_SHIFT, y << FP16_16_SHIFT};
    Actor *foe = NULL;
    u32 hits = 0;

    while ((foe = hackLib_nextTargetAt(&center, actor, radius, foe)) != NULL)
    {
        HackLibDamageResult result;

        if (!skillExpansion_isFoe(foe))
        {
            continue;
        }
        result = skillExpansion_dealDamage(actor, foe, damage, HACKLIB_DAMAGE_ABILITY, row);
        if (!result.hit)
        {
            continue;
        }
        hits++;
        hackLib_fxBurst(foe, FX_SHAPE_SPARK, color, FX_LAYER_FRONT);
        if (!result.killed && stunFrames != 0)
        {
            hackLib_applyStun(foe, stunFrames);
        }
    }
    return hits;
}

/** Stun and mark every foe within radius of (x, y). */
static void stunAt(Actor *actor, s32 x, s32 y, s32 radius, s32 stunFrames, HackLibFxColor color)
{
    Vector2Fp16 center = {x << FP16_16_SHIFT, y << FP16_16_SHIFT};
    Actor *foe = NULL;

    while ((foe = hackLib_nextTargetAt(&center, actor, radius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            hackLib_applyStun(foe, stunFrames);
            hackLib_fxBurst(foe, FX_SHAPE_SPARK, color, FX_LAYER_FRONT);
        }
    }
}

static void hide(Actor *actor, GandalfState *st, u8 frames)
{
    if (frames > st->hideFrames)
    {
        st->hideFrames = frames;
    }
    actor->flags.p |= ACTOR_FLAG_23;
}

/** Fill the White's palette blend into Gandalf's sprite slot. */
static void loadTint(Actor *actor, u8 strength)
{
    const u16 *base = ObjPalettes[GANDALF_OBJ_PALETTE];
    u16 blended[16];
    u32 i;

    for (i = 0; i < 16; i++)
    {
        s32 mixed = 0;
        u32 shift;

        for (shift = 0; shift < 15; shift += 5)
        {
            s32 from = (base[i] >> shift) & 0x1f;
            s32 to = (WhiteColors[i] >> shift) & 0x1f;

            mixed |= (from + (to - from) * strength / FX_TINT_FULL) << shift;
        }
        blended[i] = mixed;
    }
    hackLib_fxTintSlot(actor->oam.paletteNumber, blended, 0, 0, 0, 0);
}

static void showTint(Actor *actor, GandalfState *st)
{
    u8 target = st->form == FORM_WHITE ? FX_TINT_FULL : 0;
    bool changed = st->tintStrength != target;

    if (st->tintStrength < target)
    {
        st->tintStrength++;
    }
    else if (st->tintStrength > target)
    {
        st->tintStrength--;
    }
    if (st->tintFrames != 0)
    {
        st->tintFrames--;
    }
    if (changed || (st->tintStrength != 0 && st->tintFrames == 0))
    {
        loadTint(actor, st->tintStrength);
        st->tintFrames = TINT_REFRESH_FRAMES;
    }
}

static volatile u16 *hudSpiritColors(void)
{
    return (volatile u16 *)BG_PALETTE_RAM + HUD_SPIRIT_FIRST_COLOR;
}

static bool hudShows(const u16 *colors)
{
    volatile u16 *ram = hudSpiritColors();
    u32 i;

    for (i = 0; i < HUD_SPIRIT_COLOR_COUNT; i++)
    {
        if ((ram[i] & 0x7fff) != (colors[i] & 0x7fff))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void restoreHud(GandalfState *st)
{
    if (st->hudSaved && hudShows(WhiteSpiritColors))
    {
        gfx_loadBgPaletteColors(st->hudStock, HUD_SPIRIT_FIRST_COLOR, HUD_SPIRIT_COLOR_COUNT);
    }
    st->hudSaved = FALSE;
}

/** The White's Spirit bar is white and pulses back to blue when Spirit runs low. */
static void showHud(u8 playerIndex, GandalfState *st)
{
    const u16 *want;
    u32 i;

    if (playerIndex != ACTIVE_PLAYER_INDEX || g_GameFlags.d.overlay)
    {
        return;
    }
    if (st->form != FORM_WHITE)
    {
        restoreHud(st);
        return;
    }
    if (!hudShows(WhiteSpiritColors) && (!st->hudSaved || !hudShows(st->hudStock)))
    {
        // The HUD reloaded its palette, so RAM holds the stock colors again.
        for (i = 0; i < HUD_SPIRIT_COLOR_COUNT; i++)
        {
            st->hudStock[i] = hudSpiritColors()[i];
        }
        st->hudSaved = TRUE;
    }
    want = WhiteSpiritColors;
    if (PLAYER(playerIndex).currentSpirit < WHITE_LOW_SPIRIT)
    {
        st->hudPulseFrames = (st->hudPulseFrames + 1) % (2 * HUD_PULSE_FRAMES);
        if (st->hudPulseFrames >= HUD_PULSE_FRAMES)
        {
            want = st->hudStock;
        }
    }
    if (!hudShows(want))
    {
        gfx_loadBgPaletteColors(want, HUD_SPIRIT_FIRST_COLOR, HUD_SPIRIT_COLOR_COUNT);
    }
}

/** The White has no Spirit regeneration. Regen stats rebuilt while White are held back again. */
static void holdRegen(u8 playerIndex, GandalfState *st)
{
    if (PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN) != 0)
    {
        st->savedRegen = PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN);
        PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN) = 0;
    }
    if (PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN_PERCENT) != 0)
    {
        st->savedRegenPercent = PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN_PERCENT);
        PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN_PERCENT) = 0;
    }
}

static void releaseRegen(u8 playerIndex, GandalfState *st)
{
    if (st->savedRegen != 0 && PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN) == 0)
    {
        PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN) = st->savedRegen;
    }
    if (st->savedRegenPercent != 0 && PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN_PERCENT) == 0)
    {
        PLAYER_STAT(playerIndex, STAT_SPIRIT_REGEN_PERCENT) = st->savedRegenPercent;
    }
    st->savedRegen = 0;
    st->savedRegenPercent = 0;
}

/** End the haste unless the game rebuilt the speed stat meanwhile. */
static void endHaste(u8 playerIndex, GandalfState *st)
{
    if (st->hastedSpeed != 0 && PLAYER_STAT(playerIndex, STAT_SPEED_PERCENT) == st->hastedSpeed)
    {
        PLAYER_STAT(playerIndex, STAT_SPEED_PERCENT) -= FLY_SPEED_PERCENT;
        PLAYER(playerIndex).combatFlags.p |= PLAYER_COMBAT_FLAG_MOVE_SPEED_DIRTY;
    }
    st->hastedSpeed = 0;
    st->hasteFrames = 0;
}

/** Each page 2 row costs its current form's half. Old Tricks and Quickened discount it. */
static void writeCosts(u8 playerIndex, GandalfState *st)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    s32 oldTricks = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_OLD_TRICKS);
    s32 quickened = st->quickened
                        ? passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_QUICKENED) *
                              QUICKENED_PERCENT_PER_LEVEL
                        : 0;
    u32 row;

    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        s32 cost;

        if (row < ACTIVE_SKILL_HERBAL_HEALING)
        {
            cost = ActiveSkillsGandalf[row].spiritCost;
        }
        else if (row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW &&
                 sx->activeDefs[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW] != NULL)
        {
            u32 i = row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW;

            cost = st->form == FORM_WHITE
                       ? WhiteCosts[i]
                       : GreyCosts[i] * (100 - OLD_TRICKS_PERCENT_PER_LEVEL * oldTricks) / 100;
        }
        else
        {
            continue;
        }
        sx->activeSkills[row].spiritCost = cost * (100 - quickened) / 100;
    }
}

/** Fire nova of Servant of the Secret Fire, on every swap. */
static void secretFire(u8 playerIndex, Actor *actor)
{
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SECRET_FIRE) == 0)
    {
        return;
    }
    castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_FIREBURST_PROC);
    hitAt(actor, pixel(actor->coords.x), pixel(actor->coords.y), SECRET_FIRE_RADIUS,
          SECRET_FIRE_BASE_DAMAGE + PLAYER(playerIndex).level + 1, 0, HACKLIB_ABILITY_NONE,
          FX_COLOR_EMBER);
}

static void startSwapCooldown(u8 playerIndex, GandalfState *st)
{
    s32 frames = SWAP_COOLDOWN_FRAMES - QUICKENED_FRAMES_PER_LEVEL *
                                            passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_QUICKENED);

    st->swapCooldownFrames = frames > 0 ? frames : 0;
    st->quickened = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_QUICKENED) != 0;
    st->drainUnits = 0;
    st->lowWarned = FALSE;
    swapLearned[playerIndex] = TRUE;
}

/** Grey to White: the Return flashes and stuns foes nearby. */
static void becomeWhite(u8 playerIndex, Actor *actor, GandalfState *st, const char *message)
{
    s32 kindler = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_KINDLER_OF_HEARTS);
    HackLibTargetQuery query = {.center = actor,
                                .radius = RETURN_RADIUS + KINDLER_RADIUS_PER_LEVEL * kindler,
                                .filter = isFoeTarget};
    Actor *foe = NULL;

    st->form = FORM_WHITE;
    startSwapCooldown(playerIndex, st);
    holdRegen(playerIndex, st);
    while ((foe = hackLib_nextTarget(&query, foe)) != NULL)
    {
        hackLib_applyStun(foe, RETURN_STUN_FRAMES + KINDLER_FRAMES_PER_LEVEL * kindler);
    }
    castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
    castFx(actor, FX_SHAPE_WIDE_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_SUNBURST);
    secretFire(playerIndex, actor);
    skillExpansion_feedback(playerIndex, message);
}

/** White to Grey: a puff of smoke and foes lose sight of Gandalf. */
static void becomeGrey(u8 playerIndex, Actor *actor, GandalfState *st, const char *message)
{
    s32 kindler = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_KINDLER_OF_HEARTS);

    st->form = FORM_GREY;
    startSwapCooldown(playerIndex, st);
    releaseRegen(playerIndex, st);
    hide(actor, st, SMOKE_HIDE_FRAMES + KINDLER_FRAMES_PER_LEVEL * kindler);
    castFx(actor, FX_SHAPE_SMOKE_RISE, FX_COLOR_WHITE, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_PLAYER_DODGE);
    secretFire(playerIndex, actor);
    if (message != NULL)
    {
        skillExpansion_feedback(playerIndex, message);
    }
}

static bool busy(u8 playerIndex, GandalfState *st)
{
    return (PLAYER(playerIndex).activeSkillStateFlags & ACTIVE_SKILL_STATE_FLAG_CASTING) != 0 ||
           st->fallFrames != 0 || st->passFrames != 0 || st->pipeFrames != 0;
}

static void trySwap(u8 playerIndex, Actor *actor, GandalfState *st)
{
    st->swapRequested = FALSE;
    if (busy(playerIndex, st) || st->swapCooldownFrames != 0)
    {
        sfx_play(SFX_MENU_ERROR);
        return;
    }
    if (st->form == FORM_WHITE)
    {
        becomeGrey(playerIndex, actor, st, "The Grey Pilgrim");
        return;
    }
    if (PLAYER(playerIndex).currentSpirit < WHITE_MIN_SPIRIT)
    {
        sfx_play(SFX_MENU_ERROR);
        skillExpansion_feedback(playerIndex, "Not enough Spirit");
        return;
    }
    becomeWhite(playerIndex, actor, st, "Gandalf the White!");
}

/** The Fall: Gandalf is hidden and unhurt, then returns as the White with full Spirit. */
static void startFall(Actor *actor, GandalfState *st)
{
    st->fallFrames = FALL_FRAMES;
    hide(actor, st, FALL_FRAMES);
    castFx(actor, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    castFx(actor, FX_SHAPE_SMOKE_RISE, FX_COLOR_NIGHT, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
}

static void tickFall(u8 playerIndex, Actor *actor, GandalfState *st)
{
    if (st->fallFrames == 0 || --st->fallFrames != 0)
    {
        return;
    }
    castFx(actor, FX_SHAPE_LIGHTNING, FX_COLOR_WHITE, FX_LAYER_FRONT);
    player_addSpirit(playerIndex, PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT));
    becomeWhite(playerIndex, actor, st, "I have returned");
    st->swapCooldownFrames = 0;
    st->drainPauseFrames = RETURNED_DRAIN_PAUSE_FRAMES;
}

static void tickWhite(u8 playerIndex, Actor *actor, GandalfState *st)
{
    s32 unbending = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNBENDING);

    holdRegen(playerIndex, st);
    if (st->drainPauseFrames != 0)
    {
        st->drainPauseFrames--;
    }
    else
    {
        st->drainUnits += DRAIN_TENTHS - DRAIN_TENTHS_PER_UNBENDING * unbending;
        if (st->drainUnits >= DRAIN_STEP)
        {
            st->drainUnits -= DRAIN_STEP;
            player_addSpirit(playerIndex, -1);
        }
    }
    if (PLAYER(playerIndex).currentSpirit <= 0)
    {
        becomeGrey(playerIndex, actor, st, "The light fades");
        return;
    }
    if (PLAYER(playerIndex).currentSpirit < WHITE_LOW_SPIRIT)
    {
        if (!st->lowWarned)
        {
            st->lowWarned = TRUE;
            skillExpansion_feedback(playerIndex, "The light dims");
        }
    }
    else
    {
        st->lowWarned = FALSE;
    }
    if (st->sparkleFrames == 0 || --st->sparkleFrames == 0)
    {
        st->sparkleFrames = WHITE_SPARKLE_FRAMES;
        hackLib_fxBurst(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    }
}

/** Wandering Wizard: the Grey gains a Spirit every 3s -0.4s/level. */
static void tickGrey(u8 playerIndex, GandalfState *st)
{
    s32 wandering = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WANDERING_WIZARD);

    releaseRegen(playerIndex, st);
    if (wandering == 0)
    {
        return;
    }
    if (++st->drainUnits >= WANDERING_BASE_FRAMES - WANDERING_FRAMES_PER_LEVEL * wandering)
    {
        st->drainUnits = 0;
        if (PLAYER(playerIndex).currentSpirit < PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT))
        {
            player_addSpirit(playerIndex, 1);
        }
    }
}

static void tickPops(Actor *actor, GandalfState *st)
{
    static const HackLibFxColor PopColors[FIREWORKS_POPS] = {FX_COLOR_GOLD, FX_COLOR_GREEN,
                                                             FX_COLOR_EMBER};

    if (st->popsLeft == 0 || --st->popFrames != 0)
    {
        return;
    }
    st->popsLeft--;
    st->popFrames = FIREWORKS_POP_FRAMES;
    fxAtPixel(actor, st->popX, st->popY, FX_SHAPE_STAR_BLOOM, PopColors[st->popsLeft]);
    sfx_play(SFX_COMBAT_HIT_MISSILE);
    hitAt(actor, st->popX, st->popY, FIREWORKS_RADIUS,
          FIREWORKS_POP_BASE_DAMAGE + FIREWORKS_POP_DAMAGE_PER_LEVEL * st->popLevel, 0, st->popRow,
          PopColors[st->popsLeft]);
}

/** Return Gandalf to his mark and give back the HP and Spirit he had there. */
static void snapBack(u8 playerIndex, Actor *actor, GandalfState *st)
{
    Vector2Fp16 preferred = {st->markX << FP16_16_SHIFT, st->markY << FP16_16_SHIFT};
    Vector2Fp16 landing;

    if (!hackLib_findLanding(actor, &preferred, &landing))
    {
        skillExpansion_feedback(playerIndex, "The way back is shut");
        return;
    }
    castFx(actor, FX_SHAPE_SMOKE_RISE, FX_COLOR_WHITE, FX_LAYER_FRONT);
    actor->renderCoords.x += landing.x - actor->coords.x;
    actor->renderCoords.y += landing.y - actor->coords.y;
    actor->coords = landing;
    actor->velocity.x = actor->velocity.y = 0;
    if (PLAYER(playerIndex).currentHp < st->markHp)
    {
        player_addHp(playerIndex, st->markHp - PLAYER(playerIndex).currentHp, FALSE);
    }
    if (PLAYER(playerIndex).currentSpirit < st->markSpirit)
    {
        player_addSpirit(playerIndex, st->markSpirit - PLAYER(playerIndex).currentSpirit);
    }
    castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_SUNBURST);
    skillExpansion_feedback(playerIndex, "Precisely when I mean to");
}

static void tickMark(u8 playerIndex, Actor *actor, GandalfState *st)
{
    if (st->markPulses == 0 || --st->markFrames != 0)
    {
        return;
    }
    st->markFrames = NEVER_LATE_PULSE_FRAMES;
    fxAtPixel(actor, st->markX, st->markY, FX_SHAPE_CHECKER_RING, FX_COLOR_WHITE);
    if (--st->markPulses == 0)
    {
        snapBack(playerIndex, actor, st);
    }
}

static void tickZones(Actor *actor, GandalfState *st)
{
    if (st->smokeFrames != 0)
    {
        if (--st->smokeFrames % ZONE_PULSE_FRAMES == 0)
        {
            fxAtPixel(actor, st->smokeX, st->smokeY, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE);
        }
    }
    if (st->brokenFrames != 0)
    {
        if (--st->brokenFrames % ZONE_PULSE_FRAMES == 0)
        {
            Vector2Fp16 center = {st->brokenX << FP16_16_SHIFT, st->brokenY << FP16_16_SHIFT};
            Actor *foe = NULL;

            while ((foe = hackLib_nextTargetAt(&center, actor, BROKEN_RADIUS, foe)) != NULL)
            {
                if (skillExpansion_isFoe(foe))
                {
                    hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
                }
            }
        }
    }
}

static void tickPipe(u8 playerIndex, Actor *actor, GandalfState *st)
{
    if (st->pipeFrames == 0)
    {
        return;
    }
    hide(actor, st, PIPE_HIDE_FRAMES);
    if (--st->pipeFrames % PIPE_TICK_FRAMES == 0)
    {
        player_addSpirit(playerIndex, (PIPE_BASE_SPIRIT + PIPE_SPIRIT_PER_LEVEL * st->pipeLevel) /
                                          (FRAMES_PER_SECOND / PIPE_TICK_FRAMES));
        castFx(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE, FX_LAYER_FRONT);
    }
}

static void tickPass(u8 playerIndex, Actor *actor, GandalfState *st)
{
    Vector2Fp16 center = {st->passX << FP16_16_SHIFT, st->passY << FP16_16_SHIFT};
    s32 originX = pixel(actor->coords.x);
    s32 originY = pixel(actor->coords.y);
    Actor *foe = NULL;

    if (st->passFrames == 0)
    {
        return;
    }
    if (--st->passFrames % PASS_PUSH_FRAMES == 0)
    {
        while ((foe = hackLib_nextTargetAt(&center, actor, PASS_HOLD_RADIUS, foe)) != NULL)
        {
            if (skillExpansion_isFoe(foe))
            {
                pushFoe(foe, originX, originY, PASS_PUSH);
                hackLib_applyStun(foe, PASS_PUSH_FRAMES);
            }
        }
        if (st->passFrames % ZONE_PULSE_FRAMES == 0)
        {
            fxAtPixel(actor, st->passX, st->passY, FX_SHAPE_CHECKER_RING, FX_COLOR_WHITE);
        }
    }
    if (st->passFrames != 0)
    {
        return;
    }
    fxAtPixel(actor, st->passX, st->passY, FX_SHAPE_LIGHTNING, FX_COLOR_WHITE);
    fxAtPixel(actor, st->passX, st->passY, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    hitAt(actor, st->passX, st->passY, PASS_BURST_RADIUS,
          PASS_BASE_DAMAGE + PASS_DAMAGE_PER_LEVEL * st->passLevel, PASS_BURST_STUN_FRAMES,
          st->passRow, FX_COLOR_WHITE);
    startFall(actor, st);
    skillExpansion_feedback(playerIndex, "Fly, you fools...");
}

static void tickHide(Actor *actor, GandalfState *st)
{
    if (st->hideFrames != 0 && --st->hideFrames == 0)
    {
        actor->flags.p &= ~ACTOR_FLAG_23;
    }
}

static void clearState(u8 playerIndex, Actor *actor)
{
    GandalfState *st = state(playerIndex);

    releaseRegen(playerIndex, st);
    endHaste(playerIndex, st);
    restoreHud(st);
    if (actor != NULL)
    {
        if (st->hideFrames != 0)
        {
            actor->flags.p &= ~ACTOR_FLAG_23;
        }
        if (st->tintStrength != 0)
        {
            loadTint(actor, 0);
        }
    }
    memory_memset(st, 0, sizeof(GandalfState));
}

void skillExpansion_gandalfReset(u8 playerIndex)
{
    if (!isGandalf(playerIndex))
    {
        return;
    }
    clearState(playerIndex, NULL);
}

void skillExpansion_gandalfCleanup(u8 playerIndex, Actor *actor)
{
    if (!isGandalf(playerIndex))
    {
        return;
    }
    clearState(playerIndex, actor);
}

void skillExpansion_gandalfInput(u8 playerIndex)
{
    GandalfState *st;
    u16 keys = PLAYER_KEYS_CURRENT(playerIndex);

    if (!isGandalf(playerIndex) || PLAYER(playerIndex).ownerActor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (st->pipeFrames != 0 && (PLAYER_KEYS_PRESSED(playerIndex) != 0 || (keys & DPAD_ANY) != 0))
    {
        st->pipeFrames = 0;
        skillExpansion_feedback(playerIndex, "Back to work");
    }
    if ((keys & R_BUTTON) == 0 || (keys & L_BUTTON) != 0)
    {
        st->holdFrames = 0;
        return;
    }
    if (st->holdFrames == 0xff || PLAYER(playerIndex).level + 1 < SWAP_UNLOCK_LEVEL)
    {
        return;
    }
    if (++st->holdFrames >= SWAP_HOLD_FRAMES)
    {
        st->holdFrames = 0xff;
        st->swapRequested = TRUE;
    }
}

void skillExpansion_gandalfTick(u8 playerIndex, Actor *actor)
{
    GandalfState *st;

    if (!isGandalf(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (PLAYER(playerIndex).currentHp <= 0)
    {
        if (st->form == FORM_WHITE || st->passFrames != 0 || st->pipeFrames != 0 ||
            st->markPulses != 0)
        {
            bool fallUsed = st->fallUsed;

            clearState(playerIndex, actor);
            state(playerIndex)->fallUsed = fallUsed;
        }
        return;
    }
    if (!st->hinted && PLAYER(playerIndex).level + 1 >= SWAP_UNLOCK_LEVEL &&
        !swapLearned[playerIndex])
    {
        st->hinted = TRUE;
        skillExpansion_feedback(playerIndex, "Hold R: Gandalf the White");
    }
    if (st->swapCooldownFrames != 0)
    {
        st->swapCooldownFrames--;
    }
    if (st->quickened &&
        (PLAYER(playerIndex).activeSkillStateFlags & ACTIVE_SKILL_STATE_FLAG_CASTING) != 0)
    {
        st->quickened = FALSE;
    }
    if (st->swapRequested)
    {
        trySwap(playerIndex, actor, st);
    }
    if (st->form == FORM_WHITE)
    {
        tickWhite(playerIndex, actor, st);
    }
    else
    {
        tickGrey(playerIndex, st);
    }
    if (st->hasteFrames != 0 && --st->hasteFrames == 0)
    {
        endHaste(playerIndex, st);
    }
    tickPops(actor, st);
    tickMark(playerIndex, actor, st);
    tickZones(actor, st);
    tickPipe(playerIndex, actor, st);
    tickPass(playerIndex, actor, st);
    tickFall(playerIndex, actor, st);
    tickHide(actor, st);
    writeCosts(playerIndex, st);
    showTint(actor, st);
    showHud(playerIndex, st);
}

bool32 skillExpansion_gandalfIsWhite(void)
{
    u8 p = ACTIVE_PLAYER_INDEX;

    return isGandalf(p) && isWhite(p);
}

s32 skillExpansion_gandalfDamage(u8 playerIndex, Actor *victim, s32 damage, bool melee)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    bool spell;
    s32 percent = 0;

    if (!isGandalf(playerIndex) || victim == NULL)
    {
        return damage;
    }
    GandalfState *st = state(playerIndex);
    // Stock spells hit without a damage source.
    spell = !melee && (source == NULL || source->kind == HACKLIB_DAMAGE_ABILITY);
    if (spell && st->form == FORM_WHITE)
    {
        if (source == NULL || source->abilityRow < HERO_ACTIVE_SKILL_COUNT)
        {
            percent += WHITE_PAGE1_PERCENT;
        }
        if (st->smokeFrames != 0 && within(victim, st->smokeX, st->smokeY, SMOKE_RADIUS))
        {
            percent += SMOKE_PERCENT_PER_LEVEL * st->smokeLevel;
        }
    }
    if (st->brokenFrames != 0 && within(victim, st->brokenX, st->brokenY, BROKEN_RADIUS))
    {
        percent += BROKEN_TAKEN_PERCENT;
    }
    return damage + damage * percent / 100;
}

s32 skillExpansion_gandalfIncomingDamage(u8 playerIndex, Actor *origin, s32 damage)
{
    GandalfState *st;
    Actor *actor = PLAYER(playerIndex).ownerActor;

    if (!isGandalf(playerIndex) || damage <= 0 || actor == NULL)
    {
        return damage;
    }
    st = state(playerIndex);
    if (st->fallFrames != 0)
    {
        return 0;
    }
    if (origin != NULL && st->brokenFrames != 0 &&
        within(origin, st->brokenX, st->brokenY, BROKEN_RADIUS))
    {
        damage = damage * BROKEN_DEALT_PERCENT / 100;
    }
    if (st->form == FORM_WHITE)
    {
        damage -= damage * ROBES_PERCENT_PER_LEVEL *
                  passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ROBES_OF_WHITE) / 100;
    }
    if (damage < 1)
    {
        damage = 1;
    }
    if (st->form == FORM_GREY && !st->fallUsed && PLAYER(playerIndex).currentHp - damage <= 0 &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_THE_FALL) != 0)
    {
        st->fallUsed = TRUE;
        st->pipeFrames = 0;
        st->passFrames = 0;
        startFall(actor, st);
        skillExpansion_feedback(playerIndex, "Fly, you fools...");
        return 0;
    }
    return damage;
}

void skillExpansion_gandalfOnKill(u8 playerIndex, Actor *victim)
{
    s32 rider = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WHITE_RIDER);

    (void)victim;
    if (!isGandalf(playerIndex) || !isWhite(playerIndex) || rider == 0)
    {
        return;
    }
    player_addSpirit(playerIndex, WHITE_RIDER_SPIRIT_PER_LEVEL * rider);
}

static SkillExpansionCastResult fireworks(Actor *playerActor, u8 p, u8 level, u8 row)
{
    GandalfState *st = state(p);
    s32 x;
    s32 y;

    pointAhead(playerActor, FIREWORKS_REACH, &x, &y);
    castFx(playerActor, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
    fxAtPixel(playerActor, x, y, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD);
    fxAtPixel(playerActor, x, y, FX_SHAPE_WIDE_RING, FX_COLOR_EMBER);
    sfx_play(SFX_COMBAT_FIREBURST_PROC);
    hitAt(playerActor, x, y, FIREWORKS_RADIUS,
          FIREWORKS_BASE_DAMAGE + FIREWORKS_DAMAGE_PER_LEVEL * level, FIREWORKS_STUN_FRAMES, row,
          FX_COLOR_GOLD);
    st->popX = x;
    st->popY = y;
    st->popLevel = level;
    st->popRow = row;
    st->popsLeft = FIREWORKS_POPS;
    st->popFrames = FIREWORKS_POP_FRAMES;
    return SKILL_CAST_SUCCESS;
}

/** A beam along Gandalf's facing that pierces every foe on it. */
static SkillExpansionCastResult flameOfAnor(Actor *playerActor, u8 level, u8 row)
{
    Vector2Int facing = facingOf(playerActor);
    s32 scale = facing.x != 0 && facing.y != 0 ? DIAGONAL_SCALE : 256;
    s32 originX = pixel(playerActor->coords.x);
    s32 originY = pixel(playerActor->coords.y);
    Actor *foe = NULL;
    s32 distance;

    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
    for (distance = ANOR_FX_STEP; distance <= ANOR_LENGTH; distance += ANOR_FX_STEP)
    {
        s32 x;
        s32 y;

        pointAhead(playerActor, distance, &x, &y);
        fxAtPixel(playerActor, x, y, FX_SHAPE_LIGHTNING, FX_COLOR_WHITE);
    }
    sfx_play(SFX_COMBAT_SUNBURST);
    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        s32 dx = pixel(foe->coords.x) - originX;
        s32 dy = pixel(foe->coords.y) - originY;
        s32 along = (dx * facing.x + dy * facing.y) * scale / 256;
        s32 across = ABS(dx * facing.y - dy * facing.x) * scale / 256;

        if (!skillExpansion_isFoe(foe) || along < 0 || along > ANOR_LENGTH ||
            across > ANOR_HALF_WIDTH)
        {
            continue;
        }
        if (skillExpansion_dealDamage(playerActor, foe,
                                      ANOR_BASE_DAMAGE + ANOR_DAMAGE_PER_LEVEL * level,
                                      HACKLIB_DAMAGE_ABILITY, row)
                .hit)
        {
            hackLib_fxBurst(foe, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
    }
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gandalfFireworks(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    if (isWhite(p))
    {
        return flameOfAnor(playerActor, castLevel(p), PLAYER(p).castingActiveSkill);
    }
    return fireworks(playerActor, p, castLevel(p), PLAYER(p).castingActiveSkill);
}

/** Charge along the facing until a wall, trampling and scattering foes on the way. */
static SkillExpansionCastResult shadowfax(Actor *playerActor, u8 level, u8 row)
{
    Vector2Int facing = facingOf(playerActor);
    s32 step = facing.x != 0 && facing.y != 0 ? SHADOWFAX_STEP * DIAGONAL_SCALE / 256
                                              : SHADOWFAX_STEP;
    Vector2Fp16 start = playerActor->coords;
    Vector2Fp16 end = start;
    s32 travelled;
    s32 fxTravelled = 0;

    for (travelled = SHADOWFAX_STEP; travelled <= SHADOWFAX_REACH; travelled += SHADOWFAX_STEP)
    {
        Vector2Fp16 next = {end.x + (facing.x * step << FP16_16_SHIFT),
                            end.y + (facing.y * step << FP16_16_SHIFT)};

        if (!hackLib_canLandAt(playerActor, &next))
        {
            break;
        }
        end = next;
        if (travelled - fxTravelled >= SHADOWFAX_FX_STEP)
        {
            fxTravelled = travelled;
            fxAtPixel(playerActor, pixel(end.x), pixel(end.y), FX_SHAPE_SMOKE_SMALL,
                      FX_COLOR_WHITE);
        }
    }
    if (end.x == start.x && end.y == start.y)
    {
        return SKILL_CAST_BLOCKED_LANDING;
    }
    castFx(playerActor, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    playerActor->renderCoords.x += end.x - playerActor->coords.x;
    playerActor->renderCoords.y += end.y - playerActor->coords.y;
    playerActor->coords = end;
    playerActor->velocity.x = playerActor->velocity.y = 0;
    {
        Actor *foe = NULL;
        s32 startX = pixel(start.x);
        s32 startY = pixel(start.y);
        s32 dx = pixel(end.x) - startX;
        s32 dy = pixel(end.y) - startY;
        s32 lengthSquared = dx * dx + dy * dy;

        while ((foe = findNextEnemy(playerActor, foe)) != NULL)
        {
            s32 fx = pixel(foe->coords.x) - startX;
            s32 fy = pixel(foe->coords.y) - startY;
            s32 t = fx * dx + fy * dy;
            s32 nearX;
            s32 nearY;

            if (!skillExpansion_isFoe(foe) || t < 0 || t > lengthSquared)
            {
                continue;
            }
            nearX = startX + dx * t / lengthSquared;
            nearY = startY + dy * t / lengthSquared;
            if (!within(foe, nearX, nearY, SHADOWFAX_HIT_RADIUS))
            {
                continue;
            }
            if (skillExpansion_dealDamage(playerActor, foe,
                                          SHADOWFAX_BASE_DAMAGE + SHADOWFAX_DAMAGE_PER_LEVEL * level,
                                          HACKLIB_DAMAGE_ABILITY, row)
                    .hit &&
                foe->currentHp > 0)
            {
                pushFoe(foe, nearX, nearY, SHADOWFAX_PUSH);
            }
        }
    }
    castFx(playerActor, FX_SHAPE_RISING_STREAKS, FX_COLOR_WHITE, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gandalfNeverLate(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    GandalfState *st = state(p);

    if (isWhite(p))
    {
        return shadowfax(playerActor, castLevel(p), PLAYER(p).castingActiveSkill);
    }
    st->markX = pixel(playerActor->coords.x);
    st->markY = pixel(playerActor->coords.y);
    st->markHp = PLAYER(p).currentHp;
    st->markSpirit = PLAYER(p).currentSpirit;
    st->markPulses = NEVER_LATE_PULSES;
    st->markFrames = NEVER_LATE_PULSE_FRAMES;
    castFx(playerActor, FX_SHAPE_CHECKER_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_GANDALF_SHIELD_CAST);
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gandalfSmokeRings(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    GandalfState *st = state(p);
    s32 x;
    s32 y;

    if (isWhite(p))
    {
        st->brokenX = pixel(playerActor->coords.x);
        st->brokenY = pixel(playerActor->coords.y);
        st->brokenFrames = BROKEN_BASE_FRAMES + BROKEN_FRAMES_PER_LEVEL * level;
        castFx(playerActor, FX_SHAPE_LIGHTNING, FX_COLOR_WHITE, FX_LAYER_FRONT);
        castFx(playerActor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST, FX_LAYER_BEHIND);
        sfx_play(SFX_COMBAT_ITEM_BREAK);
        skillExpansion_feedback(p, "Your staff is broken");
        return SKILL_CAST_SUCCESS;
    }
    pointAhead(playerActor, SMOKE_REACH, &x, &y);
    st->smokeX = x;
    st->smokeY = y;
    st->smokeFrames = SMOKE_FRAMES;
    st->smokeLevel = level;
    castFx(playerActor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE, FX_LAYER_FRONT);
    fxAtPixel(playerActor, x, y, FX_SHAPE_SMOKE_RISE, FX_COLOR_WHITE);
    fxAtPixel(playerActor, x, y, FX_SHAPE_OVAL_RING, FX_COLOR_WHITE);
    sfx_play(SFX_COMBAT_EVADE);
    stunAt(playerActor, x, y, SMOKE_RADIUS, SMOKE_STUN_FRAMES, FX_COLOR_WHITE);
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gandalfPipeWeed(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    GandalfState *st = state(p);

    if (isWhite(p))
    {
        s32 percent = THEODEN_BASE_PERCENT + THEODEN_PERCENT_PER_LEVEL * level;
        u8 partner = p ^ 1;
        Actor *partnerActor = PLAYER(partner).ownerActor;

        player_addHp(p, PLAYER_STAT(p, STAT_MAX_HP) * percent / 100, FALSE);
        castFx(playerActor, FX_SHAPE_RING_SPARKLES, FX_COLOR_GOLD, FX_LAYER_FRONT);
        castFx(playerActor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GOLD, FX_LAYER_FRONT);
        if (g_GameFlags.d.coopActive && partnerActor != NULL && PLAYER(partner).currentHp > 0 &&
            within(partnerActor, pixel(playerActor->coords.x), pixel(playerActor->coords.y),
                   THEODEN_PARTNER_RADIUS))
        {
            player_addHp(partner, PLAYER_STAT(partner, STAT_MAX_HP) * percent / 100, FALSE);
            castFx(partnerActor, FX_SHAPE_RING_SPARKLES, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
        sfx_play(SFX_HERB_PICKUP);
        skillExpansion_feedback(p, "Your fingers remember");
        return SKILL_CAST_SUCCESS;
    }
    st->pipeFrames = PIPE_FRAMES;
    st->pipeLevel = level;
    hide(playerActor, st, PIPE_HIDE_FRAMES);
    castFx(playerActor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE, FX_LAYER_FRONT);
    skillExpansion_feedback(p, "A pause for pipe-weed");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gandalfFlyYouFools(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    GandalfState *st = state(p);
    Actor *foe = NULL;

    if (isWhite(p))
    {
        castFx(playerActor, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
        castFx(playerActor, FX_SHAPE_SKY_RAIN, FX_COLOR_GOLD, FX_LAYER_FRONT);
        sfx_play(SFX_COMBAT_SUNBURST);
        hitAt(playerActor, pixel(playerActor->coords.x), pixel(playerActor->coords.y),
              COMING_RADIUS, COMING_BASE_DAMAGE + COMING_DAMAGE_PER_LEVEL * level,
              COMING_STUN_FRAMES, PLAYER(p).castingActiveSkill, FX_COLOR_GOLD);
        skillExpansion_feedback(p, "Look to my coming!");
        return SKILL_CAST_SUCCESS;
    }
    {
        HackLibTargetQuery query = {
            .center = playerActor, .radius = FLY_RADIUS, .filter = isFoeTarget};

        while ((foe = hackLib_nextTarget(&query, foe)) != NULL)
        {
            pushFoe(foe, pixel(playerActor->coords.x), pixel(playerActor->coords.y), FLY_PUSH);
            hackLib_applyStun(foe, FLY_STUN_FRAMES);
        }
    }
    if (st->hastedSpeed == 0)
    {
        PLAYER_STAT(p, STAT_SPEED_PERCENT) += FLY_SPEED_PERCENT;
        PLAYER(p).combatFlags.p |= PLAYER_COMBAT_FLAG_MOVE_SPEED_DIRTY;
        st->hastedSpeed = PLAYER_STAT(p, STAT_SPEED_PERCENT);
    }
    st->hasteFrames = FLY_SPEED_FRAMES;
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    skillExpansion_feedback(p, "Fly, you fools!");
    return SKILL_CAST_SUCCESS;
}

/** Spend every Spirit Gandalf has on one screen-wide burst, then fade back to the Grey. */
static SkillExpansionCastResult lightOfValinor(Actor *playerActor, u8 p, u8 level, u8 row)
{
    GandalfState *st = state(p);
    s32 spent = skillExpansion_runtime[p].paidSpirit + PLAYER(p).currentSpirit;

    player_addSpirit(p, -PLAYER(p).currentSpirit);
    castFx(playerActor, FX_SHAPE_SKY_RAIN, FX_COLOR_WHITE, FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    hitAt(playerActor, pixel(playerActor->coords.x), pixel(playerActor->coords.y), VALINOR_RADIUS,
          spent * (VALINOR_BASE_PERCENT + VALINOR_PERCENT_PER_LEVEL * level) / 100,
          VALINOR_STUN_FRAMES, row, FX_COLOR_WHITE);
    becomeGrey(p, playerActor, st, "The light of Valinor!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gandalfShallNotPass(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    GandalfState *st = state(p);
    s32 x;
    s32 y;

    if (isWhite(p))
    {
        return lightOfValinor(playerActor, p, level, PLAYER(p).castingActiveSkill);
    }
    pointAhead(playerActor, PASS_REACH, &x, &y);
    st->passX = x;
    st->passY = y;
    st->passFrames = PASS_FRAMES;
    st->passLevel = level;
    st->passRow = PLAYER(p).castingActiveSkill;
    fxAtPixel(playerActor, x, y, FX_SHAPE_LIGHTNING, FX_COLOR_WHITE);
    fxAtPixel(playerActor, x, y, FX_SHAPE_CHECKER_RING, FX_COLOR_WHITE);
    sfx_play(SFX_COMBAT_GANDALF_SHIELD_CAST);
    skillExpansion_feedback(p, "You shall not pass!");
    return SKILL_CAST_SUCCESS;
}

bool32 skillExpansion_canCastGandalfNeverLate(u8 playerIndex)
{
    return skillExpansion_canCastGandalfSkill(playerIndex) &&
           (isWhite(playerIndex) || state(playerIndex)->markPulses == 0);
}

bool32 skillExpansion_canCastGandalfPipeWeed(u8 playerIndex)
{
    return skillExpansion_canCastGandalfSkill(playerIndex) && state(playerIndex)->pipeFrames == 0;
}

bool32 skillExpansion_canCastGandalfSkill(u8 playerIndex)
{
    GandalfState *st = state(playerIndex);

    return st->passFrames == 0 && st->fallFrames == 0;
}
