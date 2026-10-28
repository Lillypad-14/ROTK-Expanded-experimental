#if HACK_gauntlet
#include "patches/gauntlet/gauntlet_boons.h"
#endif
#include "patches/skill_expansion/skill_expansion_sam.h"
#include "fx.h"
#include "game.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "lib/skill_fx.h"
#include "lib/status.h"
#include "lib/summon.h"
#include "lib/targeting.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

#define FRAMES_PER_SECOND 30

// Cast visuals step every CAST_FX_TICKS so they read instead of flashing by.
#define CAST_FX_TICKS 3

// Samwise's Garden
#define GARDEN_SLOTS 3
#define GROWTH_PER_FRAME 10
#define GROW_FRAMES (6 * FRAMES_PER_SECOND)
#define RIPE_GROWTH (GROWTH_PER_FRAME * GROW_FRAMES)
#define TEND_RADIUS 40
#define RIPE_FRAMES (8 * FRAMES_PER_SECOND)
#define PLANT_REACH 20
// Plant and stew sprites last 20 frames and end on their first pose. Redrawing them 2 frames
// early overlaps two identical frames, so they never blink.
#define PLANT_FX_FRAMES 18
// Sow sprite length less 1, so the plant sprite starts under its last frame
#define SOW_FX_FRAMES 15
#define HARVEST_RADIUS 12
#define HARVEST_BASE_SPIRIT 6
#define HARVEST_SPIRIT_PER_LEVEL 2

// Plant Taters (Gardener)
#define TATER_PELT_FRAMES (2 * FRAMES_PER_SECOND)
#define TATER_PELT_RADIUS 40
#define TATER_PELT_BASE_DAMAGE 6
#define TATER_PELT_DAMAGE_PER_LEVEL 3
#define TATER_PELT_STUN_FRAMES FRAMES_PER_SECOND

// Bramble Hedge (Gardener)
#define BRAMBLE_PULSE_FRAMES FRAMES_PER_SECOND
#define BRAMBLE_BASE_RADIUS 20
#define BRAMBLE_RADIUS_PER_STAGE 10
#define BRAMBLE_BASE_DAMAGE 3
#define BRAMBLE_DAMAGE_PER_LEVEL 2
#define BRAMBLE_STUN_FRAMES (FRAMES_PER_SECOND / 3)

// Second Breakfast (Cook)
#define STEW_BASE_FRAMES (8 * FRAMES_PER_SECOND)
#define STEW_FRAMES_PER_LEVEL FRAMES_PER_SECOND
#define STEW_PULSE_FRAMES FRAMES_PER_SECOND
#define STEW_RADIUS 40
#define STEW_BASE_HEAL_PERCENT 2
#define STEW_HEAL_PERCENT_PER_LEVEL 1
#define STEW_BUFF_FRAMES (3 * FRAMES_PER_SECOND / 2)

// Po-tay-toes! (Cook)
#define POTATOES_AIM_RADIUS 128
#define POTATOES_BASE_DAMAGE 12
#define POTATOES_DAMAGE_PER_LEVEL 6
#define POTATOES_MIN_PERCENT 25
#define POTATOES_STUN_FRAMES FRAMES_PER_SECOND

// Elven Rope (Stout)
#define ROPE_REACH 112
#define ROPE_LANDING_DISTANCE 18
#define ROPE_BASE_DAMAGE 4
#define ROPE_DAMAGE_PER_LEVEL 2
#define ROPE_BASE_STUN_FRAMES (3 * FRAMES_PER_SECOND / 2)
#define ROPE_STUN_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 4)
#define ROPE_FX_KNOTS 3

// Frying Pan Parry (Stout)
#define PARRY_BASE_FRAMES (2 * FRAMES_PER_SECOND)
#define PARRY_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 5)
#define PARRY_MAX_CHARGES 5
#define BONK_RADIUS 36
#define BONK_BASE_DAMAGE 6
#define BONK_DAMAGE_PER_LEVEL 3
#define BONK_STUN_FRAMES FRAMES_PER_SECOND

// Passives
#define GREEN_THUMB_PERCENT_PER_LEVEL 20
#define GAFFERS_LORE_FRAMES_PER_LEVEL (2 * FRAMES_PER_SECOND)
#define LEMBAS_HEAL_PERCENT_PER_LEVEL 3
#define FINE_SEASONING_SPIRIT_PER_LEVEL 3
#define TATERS_FOR_ALL_PERCENT 20
#define STUBBORN_RADIUS 48
#define STUBBORN_PERCENT_PER_LEVEL 5
#define CARRY_HP_PERCENT 25
#define CARRY_HEAL_PERCENT 40
#define CARRY_IMMUNE_FRAMES (2 * FRAMES_PER_SECOND)
#define SHIRE_GRIT_DAMAGE_PER_LEVEL 3
#define SHIRE_GRIT_MAX_DAMAGE 99

typedef enum SamPlantKind
{
    SAM_PLANT_NONE = 0,
    SAM_PLANT_TATERS = 1,
    SAM_PLANT_BRAMBLE = 2,
    SAM_PLANT_STEW = 3,
    SAM_PLANT_WILTING = 4, // dead, wilts once its last plant sprite ends
} SamPlantKind;

typedef struct SamPlant
{
    // position in pixels
    s16 x;
    s16 y;

    // growth points, ripe at RIPE_GROWTH
    u16 growth;

    // frames left once ripe, or the stew pot's frames left
    u16 life;

    // SamPlantKind, SAM_PLANT_NONE for an empty slot
    u8 kind;

    // skill level at planting
    u8 level;

    // frames until the plant's next pelt, bramble pulse or heal
    u8 pulseFrames;

    // frames until the plant is drawn again
    u8 fxFrames;
} SamPlant;

typedef struct SamState
{
    // taters and brambles, GARDEN_SLOTS + 1 with Master Gardener
    SamPlant plants[GARDEN_SLOTS + 1];

    // Second Breakfast stew pot
    SamPlant stew;

    // Frying Pan Parry frames left
    u16 parryFrames;

    // Shire Grit damage added to the next melee hit
    u16 grit;

    // I Can Carry You immunity frames left
    u16 immuneFrames;

    // hits parried during this Frying Pan Parry
    u8 parryCharges;

    // Frying Pan Parry level at cast
    u8 parryLevel;

    // row Frying Pan Parry was cast from
    u8 parryRow;

    // row Plant Taters was cast from
    u8 taterRow;

    // row Bramble Hedge was cast from
    u8 brambleRow;

    // Taters for All damage buff frames left
    u8 buffFrames;

    // TRUE once I Can Carry You saved Sam in this zone
    bool carried;
} SamState;

_Static_assert(sizeof(SamState) <= sizeof(skillExpansion_heroState[0]),
               "SamState exceeds the hero state");

static inline SamState *state(u8 playerIndex)
{
    return (SamState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_SAM,
                                                    PLAYER(playerIndex).heroId);
}

static const Vector2Int FacingVectors[8] = {
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
};

static inline bool isSam(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_SAM &&
           skillExpansion_playerExpanded(playerIndex);
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline u8 castLevel(u8 playerIndex)
{
    return skillExpansion_effectiveActiveLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

static Actor *castFx(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(playerActor, shape, color, layer), CAST_FX_TICKS);
}

static bool32 isFoeTarget(Actor *target, u32 argument)
{
    (void)argument;
    return skillExpansion_isFoe(target);
}

static Actor *nearestFoe(Actor *center, u16 radius)
{
    HackLibTargetQuery query = {.center = center, .radius = radius, .filter = isFoeTarget};

    return hackLib_nearestTarget(&query);
}

static Vector2Fp16 plantPosition(const SamPlant *plant)
{
    Vector2Fp16 position = {plant->x << FP16_16_SHIFT, plant->y << FP16_16_SHIFT};

    return position;
}

static s32 distanceToPlant(const SamPlant *plant, const Actor *actor)
{
    s32 dx = plant->x - (actor->coords.x >> FP16_16_SHIFT);
    s32 dy = plant->y - (actor->coords.y >> FP16_16_SHIFT);

    return dx * dx + dy * dy;
}

static bool isRipe(const SamPlant *plant)
{
    return plant->growth >= RIPE_GROWTH;
}

/** 0 sprout, 1 bloom, 2 ripe. */
static u32 plantStage(const SamPlant *plant)
{
    return isRipe(plant) ? 2 : plant->growth >= RIPE_GROWTH / 2 ? 1 : 0;
}

static u32 gardenSlots(u8 playerIndex)
{
    return GARDEN_SLOTS +
           (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_MASTER_GARDENER) != 0 ? 1 : 0);
}

static Actor *plantFx(Actor *actor, const SamPlant *plant, u8 shape, HackLibFxColor color)
{
    Vector2Fp16 position = plantPosition(plant);

    return hackLib_fxAt(actor, &position, shape, color);
}

/** Play a sprite effect (HackLibEffectId) at the plant. */
static void gardenFx(Actor *actor, const SamPlant *plant, u16 effect)
{
    Vector2Fp16 position = plantPosition(plant);

    hackLib_playEffect(effect, actor, &position, NULL);
}

/** Play a sprite effect left where target stands now. */
static void spotFx(Actor *actor, const Actor *target, u16 effect)
{
    Vector2Fp16 position = target->coords;

    hackLib_playEffect(effect, actor, &position, NULL);
}

/** Damage, stun and mark with effect every foe within radius of the plant. Returns the number of
 * foes hit. */
static u32 hitAround(Actor *actor, const SamPlant *plant, s32 radius, s32 damage, s32 stunFrames,
                     u8 row, u16 effect)
{
    Actor *foe = NULL;
    u32 hits = 0;

    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        HackLibDamageResult result;

        if (!skillExpansion_isFoe(foe) || distanceToPlant(plant, foe) > radius * radius)
        {
            continue;
        }
        result = skillExpansion_dealDamage(actor, foe, damage, HACKLIB_DAMAGE_ABILITY, row);
        if (!result.hit)
        {
            continue;
        }
        hits++;
        spotFx(actor, foe, effect);
        if (!result.killed && stunFrames != 0)
        {
            hackLib_applyStun(foe, stunFrames);
        }
    }
    return hits;
}

/** The first empty slot, else the ripe plant closest to wilting, else the most grown one. */
static SamPlant *freeSlot(u8 playerIndex)
{
    SamState *st = state(playerIndex);
    SamPlant *best = NULL;
    u32 count = gardenSlots(playerIndex);
    u32 i;

    for (i = 0; i < count; i++)
    {
        SamPlant *plant = &st->plants[i];

        if (plant->kind == SAM_PLANT_NONE || plant->kind == SAM_PLANT_WILTING)
        {
            return plant;
        }
        if (best == NULL || (isRipe(plant) && (!isRipe(best) || plant->life < best->life)) ||
            (!isRipe(best) && plant->growth > best->growth))
        {
            best = plant;
        }
    }
    return best;
}

/** Plant a seed a step in front of Sam. */
static void sow(Actor *actor, u8 kind)
{
    u8 p = actor->as.combat.playerIndex;
    SamPlant *plant = freeSlot(p);
    Direction8 facing = actor->facing < 8 ? actor->facing : DIRECTION8_SOUTH;
    Vector2Fp16 preferred = actor->coords;
    Vector2Fp16 spot;

    preferred.x += FacingVectors[facing].x * (PLANT_REACH << FP16_16_SHIFT);
    preferred.y += FacingVectors[facing].y * (PLANT_REACH << FP16_16_SHIFT);
    if (!hackLib_findLanding(actor, &preferred, &spot))
    {
        spot = actor->coords;
    }
    memory_memset(plant, 0, sizeof(*plant));
    plant->x = spot.x >> FP16_16_SHIFT;
    plant->y = spot.y >> FP16_16_SHIFT;
    plant->kind = kind;
    plant->level = castLevel(p);
    plant->pulseFrames = kind == SAM_PLANT_TATERS ? TATER_PELT_FRAMES : BRAMBLE_PULSE_FRAMES;
    plant->fxFrames = SOW_FX_FRAMES;
    gardenFx(actor, plant, HACKLIB_EFFECT_SAM_SOW);
    sfx_play(SFX_HERB_PICKUP);
}

/** Kill the plant. The wilt sprite waits for the current plant sprite so the two never overlap. */
static void wilt(SamPlant *plant)
{
    plant->kind = SAM_PLANT_WILTING;
}

static void tickWilt(Actor *actor, SamPlant *plant)
{
    if (plant->fxFrames == 0 || --plant->fxFrames == 0)
    {
        gardenFx(actor, plant, HACKLIB_EFFECT_SAM_WILT);
        plant->kind = SAM_PLANT_NONE;
    }
}

/** Restore Spirit and, with Lembas Crumbs, HP for a harvested or thrown tater. */
static void reap(u8 playerIndex, Actor *actor, const SamPlant *plant, bool harvested)
{
    s32 lembas = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_LEMBAS_CRUMBS);
    s32 spirit = FINE_SEASONING_SPIRIT_PER_LEVEL *
                 passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FINE_SEASONING);

    if (harvested)
    {
        spirit += HARVEST_BASE_SPIRIT + HARVEST_SPIRIT_PER_LEVEL * plant->level;
        hackLib_fxBurst(actor, FX_SHAPE_RING_SPARKLES, FX_COLOR_GOLD, FX_LAYER_FRONT);
    }
    if (spirit != 0)
    {
        player_addSpirit(playerIndex, spirit);
    }
    if (harvested && lembas != 0)
    {
        s32 heal =
            PLAYER_STAT(playerIndex, STAT_MAX_HP) * LEMBAS_HEAL_PERCENT_PER_LEVEL * lembas / 100;

        player_addHp(playerIndex, heal > 0 ? heal : 1, FALSE);
    }
}

static void drawPlant(Actor *actor, const SamPlant *plant)
{
    static const u16 TaterSprites[3] = {HACKLIB_EFFECT_SAM_TATER_SPROUT,
                                        HACKLIB_EFFECT_SAM_TATER_LEAFY,
                                        HACKLIB_EFFECT_SAM_TATER_RIPE};
    static const u16 BrambleSprites[3] = {HACKLIB_EFFECT_SAM_BRAMBLE_SPROUT,
                                          HACKLIB_EFFECT_SAM_BRAMBLE_BUSH,
                                          HACKLIB_EFFECT_SAM_BRAMBLE_HEDGE};
    u32 stage = plantStage(plant);

    gardenFx(actor, plant,
             plant->kind == SAM_PLANT_TATERS ? TaterSprites[stage] : BrambleSprites[stage]);
}

static void tickPlant(u8 playerIndex, SamState *st, Actor *actor, SamPlant *plant)
{
    u32 stage;

    if (!isRipe(plant))
    {
        s32 rate = GROWTH_PER_FRAME *
                   (100 + GREEN_THUMB_PERCENT_PER_LEVEL *
                              passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_GREEN_THUMB)) /
                   100;
        s32 growth;

        if (distanceToPlant(plant, actor) <= TEND_RADIUS * TEND_RADIUS)
        {
            rate *= 2;
        }
        growth = plant->growth + rate;
        if (growth >= RIPE_GROWTH)
        {
            growth = RIPE_GROWTH;
            plant->life =
                RIPE_FRAMES + GAFFERS_LORE_FRAMES_PER_LEVEL *
                                  passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_GAFFERS_LORE);
            plantFx(actor, plant, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD);
        }
        plant->growth = growth;
    }
    else if (--plant->life == 0)
    {
        wilt(plant);
        return;
    }
    if (plant->kind == SAM_PLANT_TATERS && isRipe(plant) &&
        distanceToPlant(plant, actor) <= HARVEST_RADIUS * HARVEST_RADIUS)
    {
        gardenFx(actor, plant, HACKLIB_EFFECT_SAM_HARVEST);
        sfx_play(SFX_HERB_PICKUP);
        reap(playerIndex, actor, plant, TRUE);
#if HACK_gauntlet
        if (playerIndex == 0 && gauntletBoons_has(GAUNTLET_BOON_SEED_BANK))
        {
            for (u32 i = 0; i < gardenSlots(playerIndex); i++)
            {
                SamPlant *other = &st->plants[i];
                if (other != plant &&
                    (other->kind == SAM_PLANT_TATERS || other->kind == SAM_PLANT_BRAMBLE) &&
                    !isRipe(other))
                {
                    u32 growth = other->growth + RIPE_GROWTH / 4;
                    // Let the ordinary growth tick initialize a newly ripe plant's lifetime.
                    other->growth = growth >= RIPE_GROWTH ? RIPE_GROWTH - 1 : growth;
                }
            }
        }
#endif
        plant->kind = SAM_PLANT_NONE;
        return;
    }
    stage = plantStage(plant);
    if (--plant->pulseFrames == 0)
    {
        if (plant->kind == SAM_PLANT_TATERS)
        {
            plant->pulseFrames = TATER_PELT_FRAMES;
            if (stage == 2)
            {
                hitAround(actor, plant, TATER_PELT_RADIUS,
                          TATER_PELT_BASE_DAMAGE + TATER_PELT_DAMAGE_PER_LEVEL * plant->level,
                          TATER_PELT_STUN_FRAMES, st->taterRow, HACKLIB_EFFECT_SAM_SPUD_HIT);
            }
        }
        else
        {
            plant->pulseFrames = BRAMBLE_PULSE_FRAMES;
            hitAround(actor, plant, BRAMBLE_BASE_RADIUS + BRAMBLE_RADIUS_PER_STAGE * stage,
                      BRAMBLE_BASE_DAMAGE + BRAMBLE_DAMAGE_PER_LEVEL * plant->level,
                      BRAMBLE_STUN_FRAMES, st->brambleRow, HACKLIB_EFFECT_SAM_THORN_JAB);
        }
    }
    if (plant->fxFrames == 0 || --plant->fxFrames == 0)
    {
        plant->fxFrames = PLANT_FX_FRAMES;
        drawPlant(actor, plant);
    }
}

static void feedNear(u8 eater, const SamPlant *stew, s32 heal)
{
    Actor *actor = PLAYER(eater).ownerActor;

    if (actor == NULL || PLAYER(eater).currentHp <= 0 ||
        distanceToPlant(stew, actor) > STEW_RADIUS * STEW_RADIUS)
    {
        return;
    }
    player_addHp(eater, heal > 0 ? heal : 1, FALSE);
    hackLib_fxBurst(actor, FX_SHAPE_RING_SPARKLES, FX_COLOR_WHITE, FX_LAYER_FRONT);
}

static void tickStew(u8 playerIndex, SamState *st, Actor *actor)
{
    SamPlant *stew = &st->stew;

    if (stew->kind == SAM_PLANT_NONE)
    {
        return;
    }
    if (--stew->life == 0)
    {
        hackLib_fxPace(plantFx(actor, stew, FX_SHAPE_SMOKE_RISE, FX_COLOR_WHITE), CAST_FX_TICKS);
        stew->kind = SAM_PLANT_NONE;
        return;
    }
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_TATERS_FOR_ALL) != 0 &&
        distanceToPlant(stew, actor) <= STEW_RADIUS * STEW_RADIUS)
    {
        st->buffFrames = STEW_BUFF_FRAMES;
    }
    if (--stew->pulseFrames == 0)
    {
        s32 percent = STEW_BASE_HEAL_PERCENT + STEW_HEAL_PERCENT_PER_LEVEL * stew->level;
        u8 partner = playerIndex ^ 1;

        stew->pulseFrames = STEW_PULSE_FRAMES;
        feedNear(playerIndex, stew, PLAYER_STAT(playerIndex, STAT_MAX_HP) * percent / 100);
        if (g_GameFlags.d.coopActive)
        {
            feedNear(partner, stew, PLAYER_STAT(partner, STAT_MAX_HP) * percent / 100);
        }
    }
    if (stew->fxFrames == 0 || --stew->fxFrames == 0)
    {
        stew->fxFrames = PLANT_FX_FRAMES;
        gardenFx(actor, stew, HACKLIB_EFFECT_SAM_STEW_POT);
    }
}

/** Every foe near Sam takes a bonk per parried hit. */
static void bonk(u8 playerIndex, SamState *st, Actor *actor)
{
    SkillExpansionBurst burst = {
        .attacker = actor,
        .center = actor,
        .radius = BONK_RADIUS,
        .damage = (BONK_BASE_DAMAGE + BONK_DAMAGE_PER_LEVEL * st->parryLevel) * st->parryCharges,
        .stunFrames = BONK_STUN_FRAMES,
        .kind = HACKLIB_DAMAGE_ABILITY,
        .abilityRow = st->parryRow,
    };

    spotFx(actor, actor, HACKLIB_EFFECT_SAM_PAN_BONK);
    castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    skillExpansion_burstDamage(&burst);
    skillExpansion_feedback(playerIndex, "Bonk!");
}

static void tickParry(u8 playerIndex, SamState *st, Actor *actor)
{
    if (st->parryFrames == 0 || --st->parryFrames != 0)
    {
        return;
    }
    if (st->parryCharges != 0)
    {
        bonk(playerIndex, st, actor);
    }
    st->parryCharges = 0;
}

void skillExpansion_samReset(u8 playerIndex)
{
    if (!isSam(playerIndex))
    {
        return;
    }
    memory_memset(state(playerIndex), 0, sizeof(SamState));
}

void skillExpansion_samCleanup(u8 playerIndex, Actor *actor)
{
    (void)actor;
    if (!isSam(playerIndex))
    {
        return;
    }
    memory_memset(state(playerIndex), 0, sizeof(SamState));
}

void skillExpansion_samTick(u8 playerIndex, Actor *actor)
{
    SamState *st;
    u32 count;
    u32 i;

    if (!isSam(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (st->immuneFrames != 0)
    {
        st->immuneFrames--;
    }
    if (st->buffFrames != 0)
    {
        st->buffFrames--;
    }
    count = gardenSlots(playerIndex);
    for (i = 0; i < ARRAY_COUNT(st->plants); i++)
    {
        SamPlant *plant = &st->plants[i];

        if (plant->kind == SAM_PLANT_NONE)
        {
            continue;
        }
        if (plant->kind == SAM_PLANT_WILTING)
        {
            tickWilt(actor, plant);
            continue;
        }
        if (i >= count)
        {
            wilt(plant);
            continue;
        }
        tickPlant(playerIndex, st, actor, plant);
    }
    tickStew(playerIndex, st, actor);
    tickParry(playerIndex, st, actor);
}

s32 skillExpansion_samDamage(u8 playerIndex, s32 damage, bool melee)
{
    SamState *st;

    if (!isSam(playerIndex))
    {
        return damage;
    }
    st = state(playerIndex);
    if (st->buffFrames != 0)
    {
        damage += damage * TATERS_FOR_ALL_PERCENT / 100;
    }
    if (melee && st->grit != 0)
    {
        damage += st->grit;
        st->grit = 0;
    }
    return damage;
}

static bool nearGarden(SamState *st, Actor *actor)
{
    u32 i;

    if (st->stew.kind != SAM_PLANT_NONE &&
        distanceToPlant(&st->stew, actor) <= STUBBORN_RADIUS * STUBBORN_RADIUS)
    {
        return TRUE;
    }
    for (i = 0; i < ARRAY_COUNT(st->plants); i++)
    {
        if (st->plants[i].kind != SAM_PLANT_NONE &&
            distanceToPlant(&st->plants[i], actor) <= STUBBORN_RADIUS * STUBBORN_RADIUS)
        {
            return TRUE;
        }
    }
    return FALSE;
}

s32 skillExpansion_samIncomingDamage(u8 playerIndex, Actor *origin, s32 damage)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;
    s32 stubborn = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STUBBORN);
    s32 maxHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);

    (void)origin;
    if (!isSam(playerIndex) || damage <= 0 || actor == NULL)
    {
        return damage;
    }
    SamState *st = state(playerIndex);
    if (st->immuneFrames != 0)
    {
        return 0;
    }
    if (st->parryFrames != 0)
    {
        s32 grit = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SHIRE_GRIT);

        if (st->parryCharges < PARRY_MAX_CHARGES)
        {
            st->parryCharges++;
        }
        if (grit != 0)
        {
            s32 total = st->grit + SHIRE_GRIT_DAMAGE_PER_LEVEL * grit;

            st->grit = total > SHIRE_GRIT_MAX_DAMAGE ? SHIRE_GRIT_MAX_DAMAGE : total;
        }
        spotFx(actor, actor, HACKLIB_EFFECT_SAM_PAN_CLANG);
        sfx_play(SFX_COMBAT_BLOCK);
        return 0;
    }
    if (stubborn != 0 && nearGarden(st, actor))
    {
        damage -= damage * STUBBORN_PERCENT_PER_LEVEL * stubborn / 100;
        if (damage < 1)
        {
            damage = 1;
        }
    }
    if (!st->carried && passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_I_CAN_CARRY_YOU) != 0 &&
        (PLAYER(playerIndex).currentHp - damage) * 100 <= maxHp * CARRY_HP_PERCENT)
    {
        st->carried = TRUE;
        st->immuneFrames = CARRY_IMMUNE_FRAMES;
        player_addHp(playerIndex, maxHp * CARRY_HEAL_PERCENT / 100, FALSE);
        castFx(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GOLD, FX_LAYER_FRONT);
        sfx_play(SFX_COMBAT_SUNBURST);
        skillExpansion_feedback(playerIndex, "I can carry you!");
        return 0;
    }
    return damage;
}

void skillExpansion_samPlantTaters(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    state(p)->taterRow = PLAYER(p).castingActiveSkill;
    sow(playerActor, SAM_PLANT_TATERS);
}

void skillExpansion_samBrambleHedge(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    state(p)->brambleRow = PLAYER(p).castingActiveSkill;
    sow(playerActor, SAM_PLANT_BRAMBLE);
}

void skillExpansion_samSecondBreakfast(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    SamPlant *stew = &state(p)->stew;

    memory_memset(stew, 0, sizeof(*stew));
    stew->x = playerActor->coords.x >> FP16_16_SHIFT;
    stew->y = playerActor->coords.y >> FP16_16_SHIFT;
    stew->kind = SAM_PLANT_STEW;
    stew->level = level;
    stew->life = STEW_BASE_FRAMES + STEW_FRAMES_PER_LEVEL * level;
    stew->pulseFrames = 1;
    sfx_play(SFX_ITEM_PICKUP);
    skillExpansion_feedback(p, "What about second breakfast?");
}

void skillExpansion_samFryingPanParry(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SamState *st = state(p);

    st->parryLevel = castLevel(p);
    st->parryRow = PLAYER(p).castingActiveSkill;
    st->parryFrames = PARRY_BASE_FRAMES + PARRY_FRAMES_PER_LEVEL * st->parryLevel;
    st->parryCharges = 0;
    hackLib_fxTimed(playerActor, FX_SHAPE_DIAMOND_RING, FX_COLOR_GHOST, st->parryFrames,
                    FX_LAYER_BEHIND);
    spotFx(playerActor, playerActor, HACKLIB_EFFECT_SAM_PAN_CLANG);
}

SkillExpansionCastResult skillExpansion_samElvenRope(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    Actor *foe = nearestFoe(playerActor, ROPE_REACH);
    Vector2Fp16 preferred;
    Vector2Fp16 landing;
    s32 dx;
    s32 dy;
    s32 span;
    s32 i;

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    dx = (foe->coords.x - playerActor->coords.x) >> FP16_16_SHIFT;
    dy = (foe->coords.y - playerActor->coords.y) >> FP16_16_SHIFT;
    span = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
    for (i = 1; i <= ROPE_FX_KNOTS; i++)
    {
        Vector2Fp16 knot = playerActor->coords;

        knot.x += (dx * i / (ROPE_FX_KNOTS + 1)) << FP16_16_SHIFT;
        knot.y += (dy * i / (ROPE_FX_KNOTS + 1)) << FP16_16_SHIFT;
        hackLib_playEffect(HACKLIB_EFFECT_SAM_ROPE_KNOT, playerActor, &knot, NULL);
    }
    HackLibDamageResult result = skillExpansion_dealDamage(
        playerActor, foe, ROPE_BASE_DAMAGE + ROPE_DAMAGE_PER_LEVEL * level, HACKLIB_DAMAGE_ABILITY,
        PLAYER(p).castingActiveSkill);
    if (result.hit && !result.killed && span > ROPE_LANDING_DISTANCE)
    {
        preferred = playerActor->coords;
        preferred.x += (dx * ROPE_LANDING_DISTANCE / span) << FP16_16_SHIFT;
        preferred.y += (dy * ROPE_LANDING_DISTANCE / span) << FP16_16_SHIFT;
        if (hackLib_findLanding(foe, &preferred, &landing))
        {
            foe->renderCoords.x += landing.x - foe->coords.x;
            foe->renderCoords.y += landing.y - foe->coords.y;
            foe->coords = landing;
            foe->velocity.x = foe->velocity.y = 0;
        }
    }
    if (result.hit && !result.killed)
    {
        hackLib_applyStun(foe, ROPE_BASE_STUN_FRAMES + ROPE_STUN_FRAMES_PER_LEVEL * level);
    }
    spotFx(playerActor, foe, HACKLIB_EFFECT_SAM_ROPE_LASSO);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_samPoTayToes(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    SamState *st = state(p);
    u32 thrown = 0;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(st->plants); i++)
    {
        SamPlant *plant = &st->plants[i];
        s32 percent;
        Actor *foe;

        if (plant->kind != SAM_PLANT_TATERS)
        {
            continue;
        }
        foe = nearestFoe(playerActor, POTATOES_AIM_RADIUS);
        if (foe == NULL)
        {
            break;
        }
        percent = plant->growth * 100 / RIPE_GROWTH;
        if (percent < POTATOES_MIN_PERCENT)
        {
            percent = POTATOES_MIN_PERCENT;
        }
        gardenFx(playerActor, plant, HACKLIB_EFFECT_SAM_HARVEST);
        HackLibDamageResult result = skillExpansion_dealDamage(
            playerActor, foe,
            (POTATOES_BASE_DAMAGE + POTATOES_DAMAGE_PER_LEVEL * level) * percent / 100,
            HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);
        if (result.hit && !result.killed && isRipe(plant))
        {
            hackLib_applyStun(foe, POTATOES_STUN_FRAMES);
        }
        spotFx(playerActor, foe, HACKLIB_EFFECT_SAM_SPUD_HIT);
        reap(p, playerActor, plant, FALSE);
        plant->kind = SAM_PLANT_NONE;
        thrown++;
    }
    if (thrown == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    skillExpansion_feedback(p, "Po-tay-toes!");
    return SKILL_CAST_SUCCESS;
}

bool32 skillExpansion_canCastSamParry(u8 playerIndex)
{
    return state(playerIndex)->parryFrames == 0;
}
