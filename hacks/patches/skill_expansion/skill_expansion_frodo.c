#include "patches/skill_expansion/skill_expansion_frodo.h"
#include "enemy.h"
#include "fx.h"
#include "game.h"
#include "lib/fx_kit.h"
#include "lib/hud_meter.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "lib/timer.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "rng.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

#define FRAMES_PER_SECOND 30
#define BURDEN_MAX 100
#define BURDEN_TIER_1_MIN 51
#define BURDEN_TIER_2_MIN 81
#define BURDEN_TIER_3_MIN 91

// Cast visuals step every CAST_FX_TICKS so they read instead of flashing by.
#define CAST_FX_TICKS 3

// Phial of Galadriel (Good)
#define PHIAL_BASE_RADIUS 48
#define PHIAL_BURDEN_PER_RADIUS 4
#define PHIAL_BURST_BASE_DAMAGE 10
#define PHIAL_TENTHS_PER_BURDEN 5
#define PHIAL_TENTHS_PER_BURDEN_PER_LEVEL 1
#define PHIAL_BURST_STUN_FRAMES (2 * FRAMES_PER_SECOND)
#define PHIAL_BASE_FRAMES (6 * FRAMES_PER_SECOND)
#define PHIAL_FRAMES_PER_LEVEL FRAMES_PER_SECOND
#define PHIAL_PULSE_FRAMES FRAMES_PER_SECOND
#define PHIAL_PULSE_BASE_DAMAGE 4
#define PHIAL_PULSE_DAMAGE_PER_LEVEL 2
#define PHIAL_PULSE_BURDEN_PER_DAMAGE 10
#define PHIAL_PULSE_STUN_FRAMES (FRAMES_PER_SECOND / 2)
#define PHIAL_PULSE_CLEANSE 5

// Elbereth! (Good)
#define ELBERETH_RADIUS 80
#define ELBERETH_BASE_DAMAGE 5
#define ELBERETH_DAMAGE_PER_LEVEL 3
#define ELBERETH_BASE_STUN_FRAMES (2 * FRAMES_PER_SECOND)
#define ELBERETH_STUN_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define ELBERETH_BASE_HEAL_PERCENT 10
#define ELBERETH_HEAL_PERCENT_PER_LEVEL 3
#define ELBERETH_CLEANSE 20

// Shadow of the Ring (Evil)
#define SHADOW_RADIUS 80
#define SHADOW_CONE_COSINE 128 // 60 degree half angle
#define SHADOW_BASE_DAMAGE 12
#define SHADOW_TENTHS_PER_BURDEN 3
#define SHADOW_TENTHS_PER_BURDEN_PER_LEVEL 1
#define SHADOW_BURDEN 15
#define SHADOW_FX_REACH 24

// Sow Discord (Misdirection)
#define DISCORD_RADIUS 96
#define DISCORD_BASE_TARGETS 2
#define DISCORD_BASE_FRAMES (5 * FRAMES_PER_SECOND)
#define DISCORD_FRAMES_PER_LEVEL FRAMES_PER_SECOND

// Smoke and Mirrors (Misdirection)
#define SMOKE_HOP_DISTANCE 40
#define SMOKE_CONFUSE_RADIUS 56
#define SMOKE_BASE_FRAMES (2 * FRAMES_PER_SECOND)
#define SMOKE_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define SMOKE_CLOAK_FRAMES (2 * FRAMES_PER_SECOND)

// Passives
#define WEIGHT_PERCENT_PER_LEVEL 2 // per 10 Burden
#define WILL_RESIST_PERCENT_PER_LEVEL 10
#define RIDDLES_FRAMES_PER_LEVEL FRAMES_PER_SECOND
#define STING_RADIUS 72
#define STING_CHECK_FRAMES 10
#define STING_DAMAGE_PER_LEVEL 2
#define STING_SPIRIT_PER_LEVEL 1
#define STING_MESSAGE_FRAMES (10 * FRAMES_PER_SECOND)
#define HUNGER_HEAL_PERCENT_PER_LEVEL 3
#define HUNGER_FX_FRAMES FRAMES_PER_SECOND
#define TRICKSY_PERCENT_PER_LEVEL 4
#define TRICKSY_FRAMES (3 * FRAMES_PER_SECOND)
#define UNWAVERING_CALM_FRAMES (10 * FRAMES_PER_SECOND)
#define UNWAVERING_DAMAGE_MULTIPLIER 3
#define UNWAVERING_STUN_FRAMES (3 * FRAMES_PER_SECOND)
#define UNWAVERING_AURA_FRAMES (2 * FRAMES_PER_SECOND)
#define CLAIM_FRAMES (10 * FRAMES_PER_SECOND)
#define CLAIM_END_BURDEN 60
#define CLAIM_WISP_FRAMES (FRAMES_PER_SECOND / 2)
#define PRECIOUS_LIES_DURATION_PERCENT 150

// Enemy kind Ring's Persuasion never turns (flag bearers).
#define PERSUASION_IMMUNE_KIND 0xd
#define SFX_PERSUASION_SUCCESS 0x73

// Frodo's sprite colors 1..15 mix toward a tint color, strength / FX_TINT_FULL.
#define FRODO_OBJ_PALETTE (0x21 + HERO_ID_FRODO)
#define TINT_FIRST_COLOR 1
#define TINT_COLOR_COUNT 15
#define TINT_REFRESH_FRAMES FRAMES_PER_SECOND
#define BURDEN_TINT_COLOR 0x0c42 // dark violet
#define BURDEN_TINT_MAX 12
#define CLAIM_TINT_STRENGTH 14
#define STING_TINT_COLOR 0x7e8c // elven blue
#define STING_TINT_STRENGTH 5
#define CONSUMED_WISP_FRAMES (3 * FRAMES_PER_SECOND / 2)

// Burden pressure. Stock drains Burden on its own, so Frodo replaces that drain: the Ring pulls
// while foes are near, and Burden only fades out of combat.
#define PRESSURE_RADIUS 128
#define PRESSURE_CHECK_FRAMES 15
#define PULL_BASE_FRAMES (4 * FRAMES_PER_SECOND / 3)
#define PULL_FRAMES_PER_WILL_LEVEL (FRAMES_PER_SECOND / 6)
#define FADE_FRAMES (3 * FRAMES_PER_SECOND)
#define NAZGUL_HIT_BURDEN 3
#define STOCK_DRAIN_COUNTDOWN 25
#define STOCK_DRAIN_COUNTDOWN_OFFSET 0x54

// Page 1 ties
#define CLOAK_CLEANSE 15
#define PERSUASION_BURDEN 5
#define KNIFE_WINDOW_FRAMES (2 * FRAMES_PER_SECOND)
#define KNIFE_HEAVY_PERCENT 10
#define KNIFE_CONSUMED_PERCENT 20

// Put On the Ring.
#define RING_STATE_WORN 2
#define RING_HOLD_FRAMES (2 * FRAMES_PER_SECOND)
// The stock Ring cuts its timer to this when Frodo attacks.
#define RING_REVEAL_FRAMES 30
#define RING_PUT_ON_BURDEN 5
#define RING_BURDEN_BASE_FRAMES (FRAMES_PER_SECOND / 2)
#define RING_BURDEN_FRAMES_PER_LEVEL 3

// Set on the player by Galadriel's Cloak and the One Ring while enemies can't see them.
#define ACTOR_FLAG_HIDDEN ACTOR_FLAG_23

// Burden bar on the HUD
#define BAR_FLASH_FRAMES 8
#define BAR_PULSE_FRAMES 16

// Persuaded enemies a stock Persuasion cast can find already persuaded.
#define PERSUADED_SNAPSHOT_COUNT 16

typedef struct FrodoState
{
    // Claim It shadow form frames left
    u16 claimFrames;

    // frames spent at 0 Burden, capped at UNWAVERING_CALM_FRAMES
    u16 calmFrames;

    // Phial of Galadriel light zone frames left
    u16 phialFrames;

    // frames until the next light zone pulse
    u16 phialPulseFrames;

    // light zone center
    Vector2Fp16 phialCenter;

    // frames until the tint is reloaded
    u16 tintFrames;

    // frames until the next shadow wisp
    u16 wispFrames;

    // frames until the next Unwavering aura flash
    u16 auraFrames;

    // frames until Sting's glow is checked again
    u16 stingCheckFrames;

    // frames before Sting's glow message can show again
    u16 stingMessageFrames;

    // frames before the Ring's Hunger heal shows again
    u16 hungerFxFrames;

    // row spirit costs saved by the shadow form
    u16 savedSpiritCosts[SKILL_EXPANSION_ACTIVE_ROW_COUNT];

    // light zone radius in pixels
    u8 phialRadius;

    // light zone damage per pulse
    u8 phialDamage;

    // Burden at the previous tick
    u8 lastBurden;

    // tint strength currently loaded
    u8 tintStrength;

    // tint color currently loaded
    u16 tintColor;

    // TRUE once Claim It saved Frodo from a killing blow this mission
    bool claimSaveUsed;

    // TRUE while an Unwavering strike resolves
    bool unwaveringStrike;

    // TRUE while Sting glows
    bool stingGlowing;

    // frames until the Ring's next pull (in combat) or fade (out of combat)
    u16 pressureFrames;

    // frames until the next foe proximity check
    u16 pressureCheckFrames;

    // TRUE while a foe is within PRESSURE_RADIUS
    bool inCombat;

    // frames a thrown knife still counts as a shadow knife
    u16 knifeFrames;

    // Burden the HUD bar last showed
    u8 barBurden;

    // frames the HUD bar keeps flashing after Burden changed
    u8 barFlashFrames;

    // HUD bar pulse phase while Consumed
    u8 barPulseFrames;

    // stock Ring actor Put On the Ring keeps on, or NULL
    Actor *heldRing;

    // frames until the held Ring adds Burden
    u16 ringBurdenFrames;

    // frames between the held Ring's Burden gains
    u16 ringBurdenInterval;
} FrodoState;

_Static_assert(sizeof(FrodoState) <= sizeof(skillExpansion_heroState[0]),
               "FrodoState exceeds the hero state");

static inline FrodoState *state(u8 playerIndex)
{
    return (FrodoState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_FRODO,
                                                    PLAYER(playerIndex).heroId);
}

static const Vector2Int FacingVectors[8] = {
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
};

static inline bool isFrodo(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_FRODO;
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline u8 castLevel(u8 playerIndex)
{
    return *skillExpansion_activeLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

static inline s32 burden(u8 playerIndex)
{
    return PLAYER(playerIndex).corruption;
}

static void addBurden(u8 playerIndex, s32 amount)
{
    player_addCorruption(playerIndex, amount);
    // Large changes can skip the stock tier flag transitions.
    s32 now = burden(playerIndex);
    u32 tiers = 0;

    if (now >= BURDEN_TIER_1_MIN)
    {
        tiers |= GAME_FLAG_CORRUPTION_TIER_1;
    }
    if (now >= BURDEN_TIER_2_MIN)
    {
        tiers |= GAME_FLAG_CORRUPTION_TIER_2;
    }
    if (now >= BURDEN_TIER_3_MIN)
    {
        tiers |= GAME_FLAG_CORRUPTION_TIER_3;
    }
    g_GameFlags.p = (g_GameFlags.p & ~(GAME_FLAG_CORRUPTION_TIER_1 | GAME_FLAG_CORRUPTION_TIER_2 |
                                       GAME_FLAG_CORRUPTION_TIER_3)) |
                    tiers;
}

/** Clear Burden and return how much there was. */
static s32 purgeBurden(u8 playerIndex)
{
    s32 purged = burden(playerIndex);

    addBurden(playerIndex, -BURDEN_MAX);
    state(playerIndex)->lastBurden = 0;
    return purged;
}

static Actor *castFx(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(playerActor, shape, color, layer), CAST_FX_TICKS);
}

static bool32 isPersuaded(Actor *actor)
{
    return (actor->combatFlags.p & ACTOR_COMBAT_FLAG_PERSUADED) != 0;
}

/** An enemy Ring's Persuasion can turn, with the stock cast's exclusions. */
static bool32 isPersuadable(Actor *actor)
{
    return skillExpansion_isFoe(actor) && actor->type == ACTOR_TYPE_ENEMY &&
           actor->variant != PERSUASION_IMMUNE_KIND && !(actor->flags.p & ACTOR_FLAG_NO_FLINCH);
}

/** Turn an enemy for frames plus Riddles in the Dark, marked with gold sparkles. */
static void persuade(u8 playerIndex, Actor *enemy, s32 frames)
{
    frames += RIDDLES_FRAMES_PER_LEVEL *
              passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RIDDLES_IN_THE_DARK);
    actor_applyPersuasion(enemy, frames > 0xffff ? 0xffff : frames);
    hackLib_fxTimed(enemy, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, frames, FX_LAYER_FRONT);
}

static bool32 inRange(Actor *actor, const Vector2Fp16 *center, s32 radius)
{
    s32 dx = (actor->coords.x - center->x) >> FP16_16_SHIFT;
    s32 dy = (actor->coords.y - center->y) >> FP16_16_SHIFT;

    return dx * dx + dy * dy <= radius * radius;
}

static void startClaim(u8 playerIndex, FrodoState *st, Actor *actor)
{
    ActiveSkill *rows = skillExpansion_players[playerIndex].activeSkills;
    u32 row;

    if (st->claimFrames == 0)
    {
        for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            st->savedSpiritCosts[row] = rows[row].spiritCost;
            rows[row].spiritCost = 0;
        }
    }
    st->claimFrames = CLAIM_FRAMES;
    st->wispFrames = 0;
    skillExpansion_feedback(playerIndex, "The Ring claims you!");
    if (actor != NULL)
    {
        skillExpansion_spawnBurstFx(actor, FX_AOE_BURST_NIGHT, SFX_COMBAT_TRIPLE_DAMAGE);
        castFx(actor, FX_SHAPE_SMOKE_RISE, FX_COLOR_SHADOW, FX_LAYER_FRONT);
    }
}

static void restoreSpiritCosts(u8 playerIndex, FrodoState *st)
{
    u32 row;

    for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        skillExpansion_players[playerIndex].activeSkills[row].spiritCost =
            st->savedSpiritCosts[row];
    }
}

static void endClaim(u8 playerIndex, FrodoState *st)
{
    st->claimFrames = 0;
    restoreSpiritCosts(playerIndex, st);
    if (burden(playerIndex) > CLAIM_END_BURDEN)
    {
        addBurden(playerIndex, CLAIM_END_BURDEN - burden(playerIndex));
    }
    st->lastBurden = burden(playerIndex);
    skillExpansion_feedback(playerIndex, "The shadow passes");
}

/** Hold the stock Burden drain off. Its countdown is refilled before it can fire. */
static void holdStockDrain(u8 playerIndex)
{
    u8 *drain = *(u8 **)g_PlayerHuds[playerIndex].field_0x6c;

    if (drain != NULL)
    {
        *(u16 *)(drain + STOCK_DRAIN_COUNTDOWN_OFFSET) = STOCK_DRAIN_COUNTDOWN;
    }
}

static bool foeNear(Actor *actor, s32 radius)
{
    HackLibTargetQuery query = {.center = actor, .radius = radius};
    Actor *foe = NULL;

    while ((foe = hackLib_nextTarget(&query, foe)) != NULL)
    {
        // Snared, stunned and turned foes don't press the Ring.
        if (skillExpansion_isFoe(foe) && foe->actionState != ACTOR_STATE_STUNNED &&
            !isPersuaded(foe))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/** The Ring pulls Burden up while foes are near and lets it fade out of combat. */
static void tickBurdenPressure(u8 playerIndex, FrodoState *st, Actor *actor)
{
    holdStockDrain(playerIndex);
    if (hackLib_tickTimer(&st->pressureCheckFrames) || st->pressureCheckFrames == 0)
    {
        bool near = foeNear(actor, PRESSURE_RADIUS);

        st->pressureCheckFrames = PRESSURE_CHECK_FRAMES;
        if (near != st->inCombat)
        {
            st->inCombat = near;
            st->pressureFrames = 0;
        }
    }
    // The worn Ring and Claim It drive Burden themselves. The Cloak hides Frodo from the Eye.
    if (PLAYER(playerIndex).ringActor != NULL || st->claimFrames != 0 ||
        (actor->flags.p & ACTOR_FLAG_HIDDEN))
    {
        return;
    }
    if (st->pressureFrames == 0)
    {
        st->pressureFrames =
            st->inCombat
                ? PULL_BASE_FRAMES +
                      PULL_FRAMES_PER_WILL_LEVEL *
                          passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STRENGTH_OF_WILL)
                : FADE_FRAMES;
        return;
    }
    if (!hackLib_tickTimer(&st->pressureFrames))
    {
        return;
    }
    if (st->inCombat)
    {
        addBurden(playerIndex, 1);
        // Strength of Will already stretched the pull, so this gain is not refunded.
        st->lastBurden = burden(playerIndex);
    }
    else if (burden(playerIndex) != 0)
    {
        addBurden(playerIndex, -1);
        st->lastBurden = burden(playerIndex);
    }
}

/** Strength of Will: give back part of every Burden gain since the last tick. */
static void resistBurden(u8 playerIndex, FrodoState *st, Actor *actor)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STRENGTH_OF_WILL);
    s32 gain = burden(playerIndex) - st->lastBurden;

    if (level != 0 && gain > 0)
    {
        s32 refund = gain * WILL_RESIST_PERCENT_PER_LEVEL * level / 100;

        if (refund > 0)
        {
            addBurden(playerIndex, -refund);
            hackLib_fxBurst(actor, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
        }
    }
}

/** Announce Burden thresholds as they are crossed. */
static void announceBurden(u8 playerIndex, FrodoState *st, Actor *actor)
{
    s32 now = burden(playerIndex);
    s32 before = st->lastBurden;

    if (now >= FRODO_BURDEN_CONSUMED && before < FRODO_BURDEN_CONSUMED)
    {
        skillExpansion_feedback(playerIndex, "The Eye turns toward you");
        hackLib_fxBurst(actor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_NIGHT, FX_LAYER_FRONT);
    }
    else if (now >= FRODO_BURDEN_HEAVY && before < FRODO_BURDEN_HEAVY)
    {
        skillExpansion_feedback(playerIndex, "The Ring grows heavy");
        hackLib_fxBurst(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_SHADOW, FX_LAYER_FRONT);
    }
    else if (now == 0 && before != 0)
    {
        skillExpansion_feedback(playerIndex, "Your burden lifts");
        hackLib_fxBurst(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_WHITE, FX_LAYER_FRONT);
    }
    st->lastBurden = now;
}

/** Sting glows while a foe is near and the Ring is light. */
static void tickSting(u8 playerIndex, FrodoState *st, Actor *actor)
{
    HackLibTargetQuery query = {.center = actor, .radius = STING_RADIUS};
    Actor *foe = NULL;
    bool glowing = FALSE;

    if (!hackLib_tickTimer(&st->stingCheckFrames) && st->stingCheckFrames != 0)
    {
        return;
    }
    st->stingCheckFrames = STING_CHECK_FRAMES;
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE) != 0 &&
        burden(playerIndex) < FRODO_BURDEN_HEAVY)
    {
        while ((foe = hackLib_nextTarget(&query, foe)) != NULL)
        {
            if (skillExpansion_isFoe(foe))
            {
                glowing = TRUE;
                break;
            }
        }
    }
    if (glowing && !st->stingGlowing && st->stingMessageFrames == 0)
    {
        skillExpansion_feedback(playerIndex, "Sting glows blue!");
        st->stingMessageFrames = STING_MESSAGE_FRAMES;
    }
    st->stingGlowing = glowing;
}

/** Light zone of the Phial: pulses damage and blinds foes inside, and cleanses Frodo. */
static void tickPhial(u8 playerIndex, FrodoState *st, Actor *actor)
{
    Actor *foe = NULL;

    if (st->phialFrames == 0)
    {
        return;
    }
    st->phialFrames--;
    if (st->phialPulseFrames-- != 0)
    {
        return;
    }
    st->phialPulseFrames = PHIAL_PULSE_FRAMES;
    hackLib_fxAt(actor, &st->phialCenter, FX_SHAPE_OVAL_RING, FX_COLOR_GOLD);
    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && inRange(foe, &st->phialCenter, st->phialRadius))
        {
            skillExpansion_dealDamage(actor, foe, st->phialDamage, HACKLIB_DAMAGE_PERIODIC,
                                      HACKLIB_ABILITY_NONE);
            hackLib_applyStun(foe, PHIAL_PULSE_STUN_FRAMES);
            hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
        }
    }
    if (burden(playerIndex) != 0 && inRange(actor, &st->phialCenter, st->phialRadius))
    {
        addBurden(playerIndex, -PHIAL_PULSE_CLEANSE);
        st->lastBurden = burden(playerIndex);
    }
}

/** Unwavering: after 10s with no Burden, Frodo shines until the strike lands. */
static void tickCalm(u8 playerIndex, FrodoState *st, Actor *actor)
{
    if (burden(playerIndex) != 0)
    {
        st->calmFrames = 0;
        return;
    }
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNWAVERING) == 0)
    {
        return;
    }
    if (st->calmFrames < UNWAVERING_CALM_FRAMES)
    {
        if (++st->calmFrames == UNWAVERING_CALM_FRAMES)
        {
            skillExpansion_feedback(playerIndex, "Unwavering");
            castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
            sfx_play(SFX_COMBAT_SUNBURST);
            st->auraFrames = UNWAVERING_AURA_FRAMES;
        }
        return;
    }
    if (st->auraFrames-- == 0)
    {
        st->auraFrames = UNWAVERING_AURA_FRAMES;
        hackLib_fxBurst(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_BEHIND);
    }
}

static void loadTint(Actor *actor, FrodoState *st, u16 color, u8 strength)
{
    if (strength != st->tintStrength || color != st->tintColor ||
        (strength != 0 && hackLib_tickTimer(&st->tintFrames)))
    {
        hackLib_fxTintSlot(actor->oam.paletteNumber, ObjPalettes[FRODO_OBJ_PALETTE],
                           TINT_FIRST_COLOR, TINT_COLOR_COUNT, color, strength);
        st->tintStrength = strength;
        st->tintColor = color;
        st->tintFrames = TINT_REFRESH_FRAMES;
    }
}

/** Shadow form > Sting's glow > Burden darkening, plus shadow wisps when Consumed. */
static void showState(u8 playerIndex, Actor *actor, FrodoState *st)
{
    u16 wispFrames = 0;

    if (st->claimFrames != 0)
    {
        loadTint(actor, st, BURDEN_TINT_COLOR, CLAIM_TINT_STRENGTH);
        wispFrames = CLAIM_WISP_FRAMES;
    }
    else if (st->stingGlowing)
    {
        loadTint(actor, st, STING_TINT_COLOR, STING_TINT_STRENGTH);
    }
    else
    {
        loadTint(actor, st, BURDEN_TINT_COLOR, burden(playerIndex) * BURDEN_TINT_MAX / BURDEN_MAX);
        if (burden(playerIndex) >= FRODO_BURDEN_CONSUMED)
        {
            wispFrames = CONSUMED_WISP_FRAMES;
        }
    }
    if (wispFrames != 0 && st->wispFrames-- == 0)
    {
        st->wispFrames = wispFrames;
        hackLib_fxBurst(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    }
}

/** End the stock Ring. A timer of 1 runs out on its next tick in either worn state. */
static void takeOffRing(FrodoState *st, Actor *ring)
{
    if (ring->actionState <= RING_STATE_WORN)
    {
        ring->actionStateTimer = 1;
    }
    st->heldRing = NULL;
}

/** Keep the held Ring on until Frodo attacks, and let it feed Burden. */
static void tickHeldRing(u8 playerIndex, FrodoState *st, Actor *actor)
{
    Actor *ring = st->heldRing;

    if (ring == NULL)
    {
        return;
    }
    if (PLAYER(playerIndex).ringActor != ring)
    {
        st->heldRing = NULL;
        return;
    }
    if (ring->actionState != RING_STATE_WORN)
    {
        return;
    }
    if (ring->actionStateTimer <= RING_REVEAL_FRAMES)
    {
        st->heldRing = NULL;
        skillExpansion_feedback(playerIndex, "You are seen!");
        return;
    }
    ring->actionStateTimer = RING_HOLD_FRAMES;
    if (hackLib_tickTimer(&st->ringBurdenFrames))
    {
        st->ringBurdenFrames = st->ringBurdenInterval;
        addBurden(playerIndex, 1);
        hackLib_fxBurst(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    }
    if (burden(playerIndex) >= BURDEN_MAX)
    {
        takeOffRing(st, ring);
        skillExpansion_feedback(playerIndex, "The Ring is too heavy!");
    }
}

void skillExpansion_frodoReset(u8 playerIndex)
{
    if (!isFrodo(playerIndex))
    {
        return;
    }
    if (state(playerIndex)->claimFrames != 0)
    {
        restoreSpiritCosts(playerIndex, state(playerIndex));
    }
    memory_memset(state(playerIndex), 0, sizeof(FrodoState));
    state(playerIndex)->lastBurden = burden(playerIndex);
}

void skillExpansion_frodoCleanup(u8 playerIndex, Actor *actor)
{
    if (!isFrodo(playerIndex))
    {
        return;
    }
    if (state(playerIndex)->claimFrames != 0)
    {
        state(playerIndex)->claimFrames = 0;
        restoreSpiritCosts(playerIndex, state(playerIndex));
    }
    FrodoState *st = state(playerIndex);
    if (actor != NULL)
    {
        hackLib_fxTintSlot(actor->oam.paletteNumber, ObjPalettes[FRODO_OBJ_PALETTE],
                           TINT_FIRST_COLOR, TINT_COLOR_COUNT, BURDEN_TINT_COLOR, 0);
    }
    if (st->heldRing != NULL && PLAYER(playerIndex).ringActor == st->heldRing)
    {
        takeOffRing(st, st->heldRing);
    }
    st->phialFrames = 0;
    st->tintStrength = 0;
    st->tintColor = 0;
    st->tintFrames = 0;
    st->wispFrames = 0;
    st->stingGlowing = FALSE;
    st->calmFrames = 0;
    st->knifeFrames = 0;
    st->heldRing = NULL;
    if (PLAYER(playerIndex).currentHp <= 0)
    {
        purgeBurden(playerIndex);
    }
}

void skillExpansion_frodoTick(u8 playerIndex, Actor *actor)
{
    FrodoState *st;

    if (!isFrodo(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (PLAYER(playerIndex).currentHp <= 0)
    {
        skillExpansion_frodoCleanup(playerIndex, actor);
        return;
    }
    if (hackLib_tickTimer(&st->claimFrames))
    {
        endClaim(playerIndex, st);
    }
    else if (st->claimFrames == 0 && burden(playerIndex) >= BURDEN_MAX &&
             passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_CLAIM_IT) != 0)
    {
        startClaim(playerIndex, st, actor);
    }
    if (st->stingMessageFrames != 0)
    {
        st->stingMessageFrames--;
    }
    if (st->hungerFxFrames != 0)
    {
        st->hungerFxFrames--;
    }
    if (st->knifeFrames != 0)
    {
        st->knifeFrames--;
    }
    tickHeldRing(playerIndex, st, actor);
    tickBurdenPressure(playerIndex, st, actor);
    resistBurden(playerIndex, st, actor);
    announceBurden(playerIndex, st, actor);
    tickPhial(playerIndex, st, actor);
    tickCalm(playerIndex, st, actor);
    tickSting(playerIndex, st, actor);
    showState(playerIndex, actor, st);
}

/** Ring's Persuasion, adding Riddles in the Dark to the enemies it turns. */
static void ringsPersuasion(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 bonus =
        RIDDLES_FRAMES_PER_LEVEL * passiveLevel(p, SKILL_EXPANSION_PASSIVE_RIDDLES_IN_THE_DARK);
    Actor *before[PERSUADED_SNAPSHOT_COUNT];
    u32 count = 0;
    Actor *target = NULL;

    // The Ring's power has a price.
    addBurden(p, PERSUASION_BURDEN);
    if (bonus == 0)
    {
        skill_active_frodoRingsPersuasion(playerActor);
        return;
    }
    // The stock cast searches this radius around the player.
    while ((target = actor_iterateInRadius(playerActor, target, 80 << 16)) != NULL)
    {
        if (isPersuaded(target) && count < ARRAY_COUNT(before))
        {
            before[count++] = target;
        }
    }
    skill_active_frodoRingsPersuasion(playerActor);
    while ((target = actor_iterateInRadius(playerActor, target, 80 << 16)) != NULL)
    {
        u32 i;

        if (!isPersuaded(target))
        {
            continue;
        }
        for (i = 0; i < count && before[i] != target; i++)
        {
        }
        if (i == count)
        {
            actor_applyPersuasion(target, target->field_0x54 + bonus);
        }
    }
}

/** Galadriel's Cloak, also lifting some of the Burden. */
static void galadrielsCloak(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    skill_active_frodoGaladrielsCloak(playerActor);
    if (burden(p) != 0)
    {
        addBurden(p, -CLOAK_CLEANSE);
        state(p)->lastBurden = burden(p);
        hackLib_fxBurst(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GHOST, FX_LAYER_FRONT);
    }
}

/** Knife Toss. Knives thrown while the Burden is Heavy are shadow knives. */
static void knifeToss(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    skill_active_frodoKnifeToss(playerActor);
    state(p)->knifeFrames = burden(p) >= FRODO_BURDEN_HEAVY ? KNIFE_WINDOW_FRAMES : 0;
}

void skillExpansion_frodoWrapStockCallbacks(u8 playerIndex, ActiveSkill *rows)
{
    (void)playerIndex;
    rows[ACTIVE_SKILL_FRODO_KNIFE_TOSS].callback = knifeToss;
    rows[ACTIVE_SKILL_FRODO_GALADRIELS_CLOAK].callback = galadrielsCloak;
    rows[ACTIVE_SKILL_FRODO_RINGS_PERSUASION].callback = ringsPersuasion;
}

s32 skillExpansion_frodoDamage(u8 playerIndex, s32 damage, bool melee)
{
    FrodoState *st;
    s32 level;

    if (!isFrodo(playerIndex))
    {
        return damage;
    }
    st = state(playerIndex);
    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WEIGHT_OF_THE_RING);
    damage += damage * WEIGHT_PERCENT_PER_LEVEL * level * (burden(playerIndex) / 10) / 100;
    if (st->claimFrames != 0)
    {
        damage += damage / 2;
    }
    if (!melee && st->knifeFrames != 0)
    {
        damage += damage *
                  (burden(playerIndex) >= FRODO_BURDEN_CONSUMED ? KNIFE_CONSUMED_PERCENT
                                                                : KNIFE_HEAVY_PERCENT) /
                  100;
    }
    if (melee && st->stingGlowing)
    {
        damage += STING_DAMAGE_PER_LEVEL *
                  passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE);
    }
    if (melee && st->calmFrames >= UNWAVERING_CALM_FRAMES &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNWAVERING) != 0)
    {
        damage *= UNWAVERING_DAMAGE_MULTIPLIER;
        st->unwaveringStrike = TRUE;
    }
    return damage;
}

void skillExpansion_frodoOnHit(u8 playerIndex, Actor *victim, HackLibDamageResult result,
                               bool melee)
{
    FrodoState *st;
    s32 level;

    if (!isFrodo(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    if (!melee && st->knifeFrames != 0 && result.hit)
    {
        hackLib_fxBurst(victim, FX_SHAPE_SPARK, FX_COLOR_SHADOW, FX_LAYER_FRONT);
    }
    if (st->unwaveringStrike && melee)
    {
        st->unwaveringStrike = FALSE;
        st->calmFrames = 0;
        if (!result.killed)
        {
            hackLib_applyStun(victim, UNWAVERING_STUN_FRAMES);
        }
        skillExpansion_spawnBurstFx(victim, FX_AOE_BURST_SUN, SFX_COMBAT_CRIT_HIT);
    }

    if (result.damageDealt > 0 && burden(playerIndex) >= FRODO_BURDEN_HEAVY &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WEIGHT_OF_THE_RING) != 0)
    {
        hackLib_fxBurst(victim, FX_SHAPE_SPARK, FX_COLOR_NIGHT, FX_LAYER_FRONT);
    }

    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RINGS_HUNGER);
    if (level != 0 && result.damageDealt > 0 &&
        (burden(playerIndex) >= FRODO_BURDEN_HEAVY || st->claimFrames != 0 ||
         PLAYER(playerIndex).ringActor != NULL))
    {
        s32 heal = result.damageDealt * HUNGER_HEAL_PERCENT_PER_LEVEL * level / 100;
        Actor *actor = PLAYER(playerIndex).ownerActor;

        player_addHp(playerIndex, heal > 0 ? heal : 1, FALSE);
        if (actor != NULL && st->hungerFxFrames == 0)
        {
            st->hungerFxFrames = HUNGER_FX_FRAMES;
            hackLib_fxBurst(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_NIGHT, FX_LAYER_FRONT);
        }
    }

    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STING_GLOWS_BLUE);
    if (level != 0 && melee && st->stingGlowing && result.hit)
    {
        player_addSpirit(playerIndex, STING_SPIRIT_PER_LEVEL * level);
        hackLib_fxBurst(victim, FX_SHAPE_SPARK, FX_COLOR_BLUE, FX_LAYER_FRONT);
    }

    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_TRICKSY_HOBBIT);
    if (level != 0 && !result.killed && isPersuadable(victim) && !isPersuaded(victim) &&
        rng_rollRange(1, 100) <= TRICKSY_PERCENT_PER_LEVEL * level)
    {
        persuade(playerIndex, victim, TRICKSY_FRAMES);
        sfx_play(SFX_PERSUASION_SUCCESS);
    }
}

s32 skillExpansion_frodoIncomingDamage(u8 playerIndex, Actor *attacker, s32 damage)
{
    FrodoState *st;
    Actor *actor = PLAYER(playerIndex).ownerActor;

    if (!isFrodo(playerIndex) || damage <= 0)
    {
        return damage;
    }
    st = state(playerIndex);
    // Wraith blades weigh on the Ring. Strength of Will resists this like any gain.
    if (attacker != NULL && skillExpansion_isFoe(attacker) &&
        attacker->unitType < ENEMY_TYPE_COUNT &&
        EnemyTypeInfos[attacker->unitType].family == ENEMY_FAMILY_NAZGUL && st->claimFrames == 0)
    {
        addBurden(playerIndex, NAZGUL_HIT_BURDEN);
    }
    if (damage >= PLAYER(playerIndex).currentHp && PLAYER(playerIndex).currentHp > 0 &&
        !st->claimSaveUsed && passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_CLAIM_IT) != 0)
    {
        st->claimSaveUsed = TRUE;
        addBurden(playerIndex, BURDEN_MAX);
        st->lastBurden = burden(playerIndex);
        startClaim(playerIndex, st, actor);
        return PLAYER(playerIndex).currentHp - 1;
    }
    return damage;
}

void skillExpansion_frodoPhialOfGaladriel(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    FrodoState *st = state(p);
    u8 level = castLevel(p);
    s32 tenths = PHIAL_TENTHS_PER_BURDEN + PHIAL_TENTHS_PER_BURDEN_PER_LEVEL * level;
    s32 purged = purgeBurden(p);
    s32 damage;
    SkillExpansionBurst burst = {
        .attacker = playerActor,
        .center = playerActor,
        .stunFrames = PHIAL_BURST_STUN_FRAMES,
        .kind = HACKLIB_DAMAGE_ABILITY,
        .abilityRow = PLAYER(p).castingActiveSkill,
    };
    Actor *pool;

    // Unwavering's calm makes this Phial free.
    if (st->calmFrames >= UNWAVERING_CALM_FRAMES &&
        passiveLevel(p, SKILL_EXPANSION_PASSIVE_UNWAVERING) != 0)
    {
        player_addSpirit(
            p, skillExpansion_players[p].activeSkills[PLAYER(p).castingActiveSkill].spiritCost);
    }
    st->phialRadius = PHIAL_BASE_RADIUS + purged / PHIAL_BURDEN_PER_RADIUS;
    damage = PHIAL_PULSE_BASE_DAMAGE + PHIAL_PULSE_DAMAGE_PER_LEVEL * level +
             purged / PHIAL_PULSE_BURDEN_PER_DAMAGE;
    st->phialDamage = damage > 255 ? 255 : damage;
    st->phialFrames = PHIAL_BASE_FRAMES + PHIAL_FRAMES_PER_LEVEL * level;
    st->phialPulseFrames = PHIAL_PULSE_FRAMES;
    st->phialCenter = playerActor->coords;

    burst.radius = st->phialRadius;
    burst.damage = PHIAL_BURST_BASE_DAMAGE + purged * tenths / 10;
    skillExpansion_spawnBurstFx(playerActor, FX_AOE_BURST_SUN, SFX_COMBAT_SUNBURST);
    castFx(playerActor, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
    pool = hackLib_fxAt(playerActor, &st->phialCenter, FX_SHAPE_GROUND_POOL, FX_COLOR_GHOST);
    if (pool == NULL)
    {
        pool = hackLib_fxAt(playerActor, &st->phialCenter, FX_SHAPE_GROUND_POOL, FX_COLOR_WHITE);
    }
    hackLib_fxHold(pool, st->phialFrames);
    skillExpansion_burstDamage(&burst);
    skillExpansion_feedback(p, purged != 0 ? "The Phial burns away your burden" : "Light!");
}

void skillExpansion_frodoElbereth(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    u8 partner = p ^ 1;
    s32 heal = PLAYER_STAT(p, STAT_MAX_HP) *
               (ELBERETH_BASE_HEAL_PERCENT + ELBERETH_HEAL_PERCENT_PER_LEVEL * level) / 100;
    SkillExpansionBurst burst = {
        .attacker = playerActor,
        .center = playerActor,
        .radius = ELBERETH_RADIUS,
        .damage = ELBERETH_BASE_DAMAGE + ELBERETH_DAMAGE_PER_LEVEL * level,
        .stunFrames = ELBERETH_BASE_STUN_FRAMES + ELBERETH_STUN_FRAMES_PER_LEVEL * level,
        .kind = HACKLIB_DAMAGE_ABILITY,
        .abilityRow = PLAYER(p).castingActiveSkill,
    };

    // Starlight: a bright core with a cold ring sweeping outward.
    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_WHITE, FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    hackLib_fxBurst(playerActor, FX_SHAPE_SKY_RAIN, FX_COLOR_WHITE, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_SUNBURST);
    skillExpansion_burstDamage(&burst);
    addBurden(p, -ELBERETH_CLEANSE);
    state(p)->lastBurden = burden(p);
    player_addHp(p, heal > 0 ? heal : 1, FALSE);
    if (g_GameFlags.d.coopActive && PLAYER(partner).ownerActor != NULL &&
        PLAYER(partner).currentHp > 0)
    {
        player_addHp(partner, heal > 0 ? heal : 1, FALSE);
        hackLib_fxBurst(PLAYER(partner).ownerActor, FX_SHAPE_RING_SPARKLES, FX_COLOR_WHITE,
                        FX_LAYER_FRONT);
    }
    skillExpansion_feedback(p, "Elbereth! Gilthoniel!");
}

void skillExpansion_frodoShadowOfTheRing(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    Direction8 facing = playerActor->facing < 8 ? playerActor->facing : DIRECTION8_SOUTH;
    HackLibTargetQuery query = {
        .center = playerActor,
        .radius = SHADOW_RADIUS,
        .facing = FacingVectors[facing],
        .coneCosine = SHADOW_CONE_COSINE,
    };
    s32 tenths = SHADOW_TENTHS_PER_BURDEN + SHADOW_TENTHS_PER_BURDEN_PER_LEVEL * level;
    s32 damage = SHADOW_BASE_DAMAGE + burden(p) * tenths / 10;
    Vector2Fp16 front = playerActor->coords;
    Actor *foe = NULL;
    Actor *fx;

    // Put On the Ring: the Ring's own shadow costs nothing while it is held.
    if (state(p)->heldRing != NULL && !(g_GameFlags.p & GAME_FLAG_2))
    {
        player_addSpirit(p, PLAYER_ACTIVE_SKILL(p, PLAYER(p).castingActiveSkill).spiritCost);
    }
    front.x += FacingVectors[facing].x * (SHADOW_FX_REACH << 16);
    front.y += FacingVectors[facing].y * (SHADOW_FX_REACH << 16);
    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_SHADOW, FX_LAYER_FRONT);
    fx = hackLib_fxAt(playerActor, &front, FX_SHAPE_SHOCKWAVE, FX_COLOR_NIGHT);
    hackLib_fxPace(fx, CAST_FX_TICKS);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    while ((foe = hackLib_nextTarget(&query, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            HackLibDamageResult result =
                skillExpansion_dealDamage(playerActor, foe, damage > 0xffff ? 0xffff : damage,
                                          HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);

            if (result.hit)
            {
                hackLib_fxBurst(foe, FX_SHAPE_TENDRIL_BURST, FX_COLOR_NIGHT, FX_LAYER_FRONT);
            }
        }
    }
    addBurden(p, SHADOW_BURDEN);
}

SkillExpansionCastResult skillExpansion_frodoSowDiscord(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    bool unlimited = passiveLevel(p, SKILL_EXPANSION_PASSIVE_PRECIOUS_LIES) != 0;
    u32 left = DISCORD_BASE_TARGETS + level;
    s32 frames = DISCORD_BASE_FRAMES + DISCORD_FRAMES_PER_LEVEL * level;
    Actor *target = NULL;
    u32 turned = 0;

    if (unlimited)
    {
        frames = frames * PRECIOUS_LIES_DURATION_PERCENT / 100;
    }
    while ((target = actor_iterateInRadius(playerActor, target, DISCORD_RADIUS << 16)) != NULL &&
           (unlimited || turned < left))
    {
        if (isPersuadable(target) && !isPersuaded(target))
        {
            persuade(p, target, frames);
            hackLib_fxBurst(target, FX_SHAPE_STAR_BLOOM, FX_COLOR_GREEN, FX_LAYER_FRONT);
            turned++;
        }
    }
    if (turned == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    sfx_play(SFX_PERSUASION_SUCCESS);
    castFx(playerActor, FX_SHAPE_OVAL_RING, FX_COLOR_GREEN, FX_LAYER_BEHIND);
    skillExpansion_feedback(p, "They turn on each other!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_frodoSmokeAndMirrors(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    Direction8 facing = playerActor->facing < 8 ? playerActor->facing : DIRECTION8_SOUTH;
    s32 frames = SMOKE_BASE_FRAMES + SMOKE_FRAMES_PER_LEVEL * level;
    Vector2Fp16 origin = playerActor->coords;
    Vector2Fp16 preferred = origin;
    Vector2Fp16 landing;
    Actor *foe = NULL;
    Actor *cloak;

    preferred.x -= FacingVectors[facing].x * (SMOKE_HOP_DISTANCE << 16);
    preferred.y -= FacingVectors[facing].y * (SMOKE_HOP_DISTANCE << 16);
    if (!hackLib_findLanding(playerActor, &preferred, &landing))
    {
        return SKILL_CAST_BLOCKED_LANDING;
    }
    // The cloak stays behind in a puff of smoke, and foes around it lose track of Frodo.
    hackLib_fxAt(playerActor, &origin, FX_SHAPE_SMOKE_RISE, FX_COLOR_GREEN);
    cloak = hackLib_fxAt(playerActor, &origin, FX_SHAPE_GROUND_POOL, FX_COLOR_GREEN);
    hackLib_fxHold(cloak, SMOKE_CLOAK_FRAMES);
    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || !inRange(foe, &origin, SMOKE_CONFUSE_RADIUS))
        {
            continue;
        }
        if (isPersuadable(foe) && !isPersuaded(foe))
        {
            persuade(p, foe, frames);
        }
        else
        {
            hackLib_applyStun(foe, frames);
        }
    }
    playerActor->renderCoords.x += landing.x - origin.x;
    playerActor->renderCoords.y += landing.y - origin.y;
    playerActor->coords = landing;
    playerActor->velocity.x = playerActor->velocity.y = 0;
    hackLib_fxBurst(playerActor, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_PLAYER_DODGE);
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_frodoPutOnTheRing(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 row = PLAYER(p).castingActiveSkill;
    u8 level = castLevel(p);
    FrodoState *st = state(p);
    u8 *stockLevel = &PLAYER(p).activeSkillLevels[ACTIVE_SKILL_FRODO_THE_ONE_RING];
    u8 savedLevel = *stockLevel;
    s32 before[2];
    u32 i;

    if (PLAYER(p).ringActor != NULL)
    {
        // Taking it off is free.
        if (!(g_GameFlags.p & GAME_FLAG_2))
        {
            player_addSpirit(p, PLAYER_ACTIVE_SKILL(p, row).spiritCost);
        }
        takeOffRing(st, PLAYER(p).ringActor);
        castFx(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
        skillExpansion_feedback(p, "You take off the Ring");
        return SKILL_CAST_SUCCESS;
    }
    for (i = 0; i < g_PlayerGlobals.activePlayerCount && i < 2; i++)
    {
        before[i] = burden(i);
    }
    // The stock Ring reads its strength from its own row's level.
    *stockLevel = level != 0 ? level : 1;
    skill_active_frodoTheOneRing(playerActor);
    *stockLevel = savedLevel;
    // Undo the stock Ring's +50 for everyone. Put On the Ring charges by the second instead.
    for (i = 0; i < g_PlayerGlobals.activePlayerCount && i < 2; i++)
    {
        addBurden(i, before[i] - burden(i));
    }
    if (PLAYER(p).ringActor == NULL)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    addBurden(p, RING_PUT_ON_BURDEN);
    st->heldRing = PLAYER(p).ringActor;
    st->heldRing->actionStateTimer = RING_HOLD_FRAMES;
    st->ringBurdenInterval = RING_BURDEN_BASE_FRAMES + RING_BURDEN_FRAMES_PER_LEVEL * level;
    st->ringBurdenFrames = st->ringBurdenInterval;
    castFx(playerActor, FX_SHAPE_SMOKE_RISE, FX_COLOR_SHADOW, FX_LAYER_FRONT);
    hackLib_fxBurst(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    skillExpansion_feedback(p, "You put on the Ring");
    return SKILL_CAST_SUCCESS;
}

bool32 skillExpansion_canCastPhialOfGaladriel(u8 playerIndex)
{
    return state(playerIndex)->phialFrames == 0;
}

// Notches at the stock corruption tiers.
static const u8 BurdenBarMarks[] = {FRODO_BURDEN_HEAVY, FRODO_BURDEN_CONSUMED};

static const HackLibHudMeterLook BarLight = {HACKLIB_HUD_COLOR_PALE_GOLD, HACKLIB_HUD_COLOR_GOLD,
                                             HACKLIB_HUD_COLOR_DARK_GOLD, HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook BarHeavy = {HACKLIB_HUD_COLOR_EMBER, HACKLIB_HUD_COLOR_RED,
                                             HACKLIB_HUD_COLOR_DARK_RED, HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook BarConsumed = {HACKLIB_HUD_COLOR_RED, HACKLIB_HUD_COLOR_DARK_RED,
                                                HACKLIB_HUD_COLOR_BROWN, HACKLIB_HUD_COLOR_GOLD};
static const HackLibHudMeterLook BarClaimed = {HACKLIB_HUD_COLOR_DARK_RED, HACKLIB_HUD_COLOR_BROWN,
                                               HACKLIB_HUD_COLOR_NEAR_BLACK,
                                               HACKLIB_HUD_COLOR_EMBER};
static const HackLibHudMeterLook BarFlash = {HACKLIB_HUD_COLOR_WHITE, HACKLIB_HUD_COLOR_PALE_GOLD,
                                             HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_WHITE};

void skillExpansion_frodoDrawBurdenBar(u8 playerIndex)
{
    FrodoState *st;
    s32 now = burden(playerIndex);
    const HackLibHudMeterLook *look;

    if (!isFrodo(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    if (now != st->barBurden)
    {
        st->barBurden = now;
        st->barFlashFrames = BAR_FLASH_FRAMES;
    }
    if (++st->barPulseFrames >= 2 * BAR_PULSE_FRAMES)
    {
        st->barPulseFrames = 0;
    }
    if (st->claimFrames != 0)
    {
        look = &BarClaimed;
        now = BURDEN_MAX;
    }
    else if (st->barFlashFrames != 0)
    {
        st->barFlashFrames--;
        look = &BarFlash;
    }
    else if (now >= FRODO_BURDEN_CONSUMED)
    {
        look = st->barPulseFrames < BAR_PULSE_FRAMES ? &BarConsumed : &BarHeavy;
    }
    else
    {
        look = now >= FRODO_BURDEN_HEAVY ? &BarHeavy : &BarLight;
    }
    hackLib_drawHudMeter(&HackLibHudGapMeter, now, BURDEN_MAX, look, BurdenBarMarks,
                         ARRAY_COUNT(BurdenBarMarks));
}
