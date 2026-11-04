// The skill engine of data-written kits (spec_kit.h). Every hero with a spec shares these
// callbacks; its own tables say what each row does. hero_kit.py links those tables apart from the
// modcode into the hero's kit slot (SKILL_EXPANSION_KIT_SLOT), so this code never names them.
#include "patches/heroes/spec_kit.h"
#include "actor/lootPile.h"
#include "enemy.h"
#include "item.h"
#include "loot.h"
#include "hero_count.h"
#include "lib/combat_source.h"
#include "lib/fx_kit.h"
#include "lib/hud_meter.h"
#include "lib/skill_fx.h"
#include "lib/movement.h"
#include "lib/summon.h"
#include "lib/status.h"
#include "lib/summon_registry.h"
#include "lib/targeting.h"
#include "patches/skill_expansion/skill_status.h"
#include "math.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "player.h"
#include "sfx.h"
#include "sprite.h"
#include "stats.h"
#include "variables.h"

#if PC_EDITION && HACK_skill_expansion && HACK_ringwraith

#define POLICY(frames) {1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, frames}
const SkillExpansionCastPolicy SpecKitQuickPolicy = POLICY(12);
const SkillExpansionCastPolicy SpecKitNormalPolicy = POLICY(30);
const SkillExpansionCastPolicy SpecKitLongPolicy = POLICY(90);
const SkillExpansionCastPolicy SpecKitUltimatePolicy = POLICY(180);

// Devour leaves bosses and other foes this tough alone.
#define DEVOUR_MAX_HP 400
// Pixels between the caster and each summoned ally.
#define SUMMON_SPREAD 22
// Frames the HUD meter flashes after a charge is gained or spent.
#define METER_FLASH_FRAMES 20
// Frames between the bubbles of a rebirth in progress.
#define REBIRTH_BUBBLE_FRAMES 10
// Enemy palette id per enemy type (the palette enemies of that type are drawn in).
#define ENEMY_PALETTE_IDS ((const u8 *)0x0806d27c)
// An animated fx layer's ticks per frame when its spec names none.
#define ANIM_FX_TICKS 2
// A charging rider's ticks per animation frame.
#define CHARGE_ANIM_TICKS 2
// Foes a charge cannot stun take its hit only on every this many ticks.
#define CHARGE_REPEAT_TICKS 8

// Delayed look layers waiting to play (SpecFx.delay).
#define CUE_COUNT 8
// Charging and swooping fx actors alive at once.
#define MOVER_COUNT 8
// Zones, roots and fears still working after their cast.
#define LASTING_COUNT 16
// A zone's ticks between pulses when its spec names none.
#define ZONE_PULSE_TICKS 30
// Ticks between an intangible hero's touches on the foes around him.
#define PHASE_TOUCH_TICKS 8
// Foes that get a fear or root its look (the rest are affected without one).
#define LOOK_FOES 6
// A stampede's riders start this many pixels behind the caster, this many apart sideways.
#define STAMPEDE_BACK 40
#define STAMPEDE_SPACING 26
// A charmed foe strikes the foe next to it this often, from this close (pixels).
#define CHARM_STRIKE_TICKS 30
#define CHARM_REACH 24
// Bleed and venom stacks hurt this often.
#define STACK_TICKS 30
// A dragged foe stops this close to the caster (pixels), and a leap lands this close.
#define PULL_REACH 22
#define LEAP_REACH 16
// Idle Frenzy stacks drain one per this many ticks.
#define FRENZY_DRAIN_TICKS 10
// A backstep lasts this many ticks.
#define BACKSTEP_FRAMES 10
// The most bleed or venom stacks on one foe.
#define STACK_MAX 9
// SpriteFlags bit: drawn see-through (the stock stealth look).
#define SPRITE_FLAG_STEALTH 0x20
// A swoop starts this many pixels beside and above its strike point.
#define SWOOP_FROM_X 112
#define SWOOP_FROM_Y 96
// A swoop's flight time when its spec names none.
#define SWOOP_DEFAULT_FRAMES 18
// A swooper's ticks per animation frame once it strikes.
#define STRIKE_ANIM_TICKS 3
// Pixels between the trail layers drawn along a lash.
#define LASH_STEP 16
// A shoved foe flies this many ticks.
#define SHOVE_FRAMES 10
// A keg lands this many pixels ahead of the caster.
#define BOMB_AHEAD 20
// A bolt's look flies this many pixels a tick.
#define BOLT_SPEED 10
// A falling bolt of a rain hits foes this close to where it lands.
#define HAIL_HIT_RADIUS 18
// Fan bolts spread this many hundredths of the facing sideways per step.
#define FAN_SPREAD 30
// Second wind counts at most this many foes.
#define SECOND_WIND_FOES 5
// A haste ignores steps longer than this many pixels (a warp, not a walk).
#define HASTE_MAX_STEP 8
// playFx: the caster's layers, the target's, or both.
#define SPEC_PLAY_SELF 1
#define SPEC_PLAY_TARGET 2
#define SPEC_PLAY_ALL (SPEC_PLAY_SELF | SPEC_PLAY_TARGET)

// The art of the eight facings: up .. down, then mirrored.
static const u8 FacingArt[8] = {0, 1, 2, 3, 4, 3, 2, 1};

typedef struct SpecCue
{
    // The layer to play, NULL for a free slot.
    const SpecFx *fx;
    Vector2Fp16 at;
    // The caster's player index.
    u8 player;
    // Frames until it plays.
    u8 left;
} SpecCue;

typedef struct SpecMover
{
    // The moving fx actor, NULL for a free slot.
    Actor *fx;
    const SpecSkill *skill;
    // Swoop: the strike point.
    Vector2Fp16 to;
    // Swoop: movement per tick.
    Vector2Fp16 step;
    // Swoop: ticks until it strikes.
    u16 left;
    u8 player;
    u8 level;
    u8 row;
    // Ticks moved, for the trail.
    u8 age;
} SpecMover;

static SpecCue Cues[CUE_COUNT];
static SpecMover Movers[MOVER_COUNT];

typedef struct SpecState
{
    u16 wardFrames;
    u16 empowerFrames;
    u8 wardPercent;
    u8 empowerPercent;
    // Frames left of a rebirth in progress (untouchable), 0 when none.
    u16 rebirthFrames;
    // PLAYER.killCount at the last tick.
    u16 lastKills;
    u8 charges;
    // Kills toward the next charge.
    u8 progress;
    // lastKills holds a real count.
    u8 killsSeen;
    u8 flashFrames;
    // The row whose empower is running (its status rides on melee hits).
    u8 empowerRow;
    // The row whose phase is running, and its skill level 1..5.
    u8 phaseRow;
    u8 phaseLevel;
    // Frames left of an intangible phase, 0 when none.
    u16 phaseFrames;
    // Frames left of a dash, its row and level.
    u16 dashFrames;
    u8 dashRow;
    u8 dashLevel;
    // The dash runs backwards and hits nothing (a backstep).
    u8 dashBack;
    // Frames left of a bounty, its row and level.
    u16 bountyFrames;
    u8 bountyRow;
    u8 bountyLevel;
    // Frames left of a despair and its spell damage bonus percent.
    u16 despairFrames;
    u16 despairPercent;
} SpecState;

_Static_assert(sizeof(SpecState) <= sizeof(skillExpansion_heroState[0]), "SpecState exceeds the hero state");

static const SpecKitTable *specTable(u8 playerIndex)
{
    u8 hero = PLAYER(playerIndex).heroId;

    return hero < SKILL_EXPANSION_KIT_SLOTS ? SKILL_EXPANSION_KIT_SLOT(hero)->specRows : NULL;
}

/** The player's spec rows, or NULL for a hero without a spec kit. */
static const SpecSkill *const *specRows(u8 playerIndex)
{
    const SpecKitTable *table = specTable(playerIndex);

    return table != NULL ? table->rows : NULL;
}

static const SpecRebirth *specRebirth(u8 playerIndex)
{
    const SpecKitTable *table = specTable(playerIndex);

    return table != NULL ? table->rebirth : NULL;
}

const SkillExpansionElitePassive *specKit_passive(u32 passiveSkillId)
{
    u32 offset = passiveSkillId - SKILL_EXPANSION_SPEC_PASSIVE_FIRST;
    const SpecKitTable *table;
    const SpecPassive *passive;

    if (passiveSkillId < SKILL_EXPANSION_SPEC_PASSIVE_FIRST || passiveSkillId >= SKILL_EXPANSION_SPEC_PASSIVE_END)
    {
        return NULL;
    }
    table = specTable((u8)(offset / HERO_PASSIVE_SKILL_COUNT));
    passive = table != NULL ? table->passives[offset % HERO_PASSIVE_SKILL_COUNT] : NULL;
    return passive != NULL ? &passive->def : NULL;
}

/** The sum of the player's spec passives of an effect, at their purchased levels. */
static s32 passiveBonus(u8 playerIndex, u8 effect)
{
    const SpecKitTable *table = specTable(playerIndex);
    u32 hero = PLAYER(playerIndex).heroId;
    s32 total = 0;

    if (table == NULL || hero < SKILL_EXPANSION_SPEC_PASSIVE_FIRST_HERO ||
        playerIndex >= SKILL_EXPANSION_SPEC_PASSIVE_PLAYERS)
    {
        return 0;
    }
    for (u32 k = 0; k < HERO_PASSIVE_SKILL_COUNT; k++)
    {
        const SpecPassive *passive = table->passives[k];
        u32 id = SKILL_EXPANSION_SPEC_PASSIVE_FIRST + playerIndex * HERO_PASSIVE_SKILL_COUNT + k;
        s32 level;

        if (passive == NULL || passive->effect != effect)
        {
            continue;
        }
        level = skillExpansion_getPassiveLevel(playerIndex, (u8)id);
        if (level > 0)
        {
            total += passive->amount[(level > 5 ? 5 : level) - 1];
        }
    }
    return total;
}

static u32 maxCharges(u8 playerIndex, const SpecRebirth *rebirth)
{
    u32 most = rebirth->maxCharges + passiveBonus(playerIndex, SPEC_PASSIVE_REBIRTH_CHARGES);

    return most > 5 ? 5 : most;
}

static u32 killsPerCharge(u8 playerIndex, const SpecRebirth *rebirth)
{
    s32 kills = rebirth->killsPerCharge - passiveBonus(playerIndex, SPEC_PASSIVE_CHARGE_KILLS);

    return kills < 1 ? 1 : (u32)kills;
}

/** The player's spec state; call only for a spec hero. */
static SpecState *state(u8 playerIndex)
{
    return (SpecState *)skillExpansion_heroStateClaim(playerIndex, PLAYER(playerIndex).heroId,
                                                      PLAYER(playerIndex).heroId);
}

static bool32 layerUsed(const SpecFx *fx)
{
    return fx->color != 0 || fx->effect != 0;
}

/** One look layer spawned on `on`, left at `at` when given. */
static void playLayer(const SpecFx *fx, Actor *on, const Vector2Fp16 *at)
{
    Actor *spawned[HACKLIB_EFFECT_LAYER_LIMIT] = {NULL};
    u32 count = 0;

    if (fx->effect != 0)
    {
        HackLibEffectHandle handle = {0};

        hackLib_playEffect(fx->effect - 1, on, at, &handle);
        for (u32 i = 0; i < HACKLIB_EFFECT_LAYER_LIMIT; i++)
        {
            if (hackLib_isActorRefValid(handle.actors[i]))
            {
                spawned[count++] = handle.actors[i].actor;
            }
        }
    }
    else if (fx->anim != NULL)
    {
        spawned[count++] = hackLib_fxSprite(on, fx->anim, (HackLibFxColor)fx->color,
                                            fx->ticks != 0 ? fx->ticks : ANIM_FX_TICKS, fx->layer);
    }
    else if (fx->frames != 0)
    {
        spawned[count++] = hackLib_fxTimed(on, fx->shape, (HackLibFxColor)fx->color, fx->frames, fx->layer);
    }
    else
    {
        spawned[count++] = hackLib_fxBurst(on, fx->shape, (HackLibFxColor)fx->color, fx->layer);
    }
    for (u32 i = 0; i < count; i++)
    {
        if (spawned[i] == NULL)
        {
            continue;
        }
        if (at != NULL && fx->effect == 0)
        {
            hackLib_fxDetach(spawned[i], at);
        }
        if (fx->frames != 0 && (fx->anim != NULL || fx->effect != 0))
        {
            hackLib_fxHold(spawned[i], fx->frames);
        }
    }
}

/** Queue a delayed layer at a spot. A full queue plays it at once. */
static void cueLayer(const SpecFx *fx, u8 player, Actor *on, const Vector2Fp16 *at)
{
    for (u32 i = 0; i < CUE_COUNT; i++)
    {
        if (Cues[i].fx == NULL)
        {
            Cues[i].fx = fx;
            Cues[i].at = *at;
            Cues[i].player = player;
            Cues[i].left = fx->delay;
            return;
        }
    }
    playLayer(fx, on, at);
}

static void tickCues(u8 player, Actor *actor)
{
    for (u32 i = 0; i < CUE_COUNT; i++)
    {
        SpecCue *cue = &Cues[i];

        if (cue->fx != NULL && cue->player == player && --cue->left == 0)
        {
            const SpecFx *fx = cue->fx;

            cue->fx = NULL;
            playLayer(fx, actor, &cue->at);
        }
    }
}

/**
 * The skill's look: which (SPEC_PLAY_*) picks the caster's layers, the target's or both. Target
 * layers play on target, or at targetAt when there is no target actor.
 */
static void playFx(const SpecSkill *skill, Actor *caster, Actor *target, const Vector2Fp16 *targetAt, u32 which)
{
    for (u32 i = 0; i < SPEC_FX_COUNT; i++)
    {
        const SpecFx *fx = &skill->fx[i];
        bool32 atTarget = (fx->flags & SPEC_FX_AT_TARGET) != 0;
        Actor *on = atTarget && target != NULL ? target : caster;
        const Vector2Fp16 *at = atTarget && target == NULL ? targetAt : NULL;

        if (!layerUsed(fx) || !(which & (atTarget ? SPEC_PLAY_TARGET : SPEC_PLAY_SELF)) ||
            (atTarget && target == NULL && targetAt == NULL))
        {
            continue;
        }
        if (fx->delay != 0)
        {
            cueLayer(fx, caster->as.combat.playerIndex, caster, at != NULL ? at : &on->coords);
        }
        else
        {
            playLayer(fx, on, at);
        }
    }
}

static bool32 launch(Actor *actor, u8 playerIndex, const SpecSkill *skill, u8 level, u8 row,
                     Actor **outTarget)
{
    Vector2Fp16 at = actor->coords;
    u8 direction = actor->facing < 8 ? actor->facing * 4 : 0;

    *outTarget = NULL;
    if (skill->targetRange != 0)
    {
        Actor *target = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);
        Vector2Fp16 toward;

        if (target == NULL)
        {
            return FALSE;
        }
        *outTarget = target;
        at = target->coords;
        toward.x = target->coords.x - actor->coords.x;
        toward.y = target->coords.y - actor->coords.y;
        direction = math_vector2_direction32(toward);
    }
    return skillDelivery_launch(playerIndex, skill->delivery, &at, direction, level, row, FALSE);
}

static bool32 devourable(Actor *foe, u32 threshold)
{
    u32 maxHp;

    if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
    {
        return FALSE;
    }
    maxHp = combat_getEnemyMaxHp(foe->unitType);
    return maxHp != 0 && maxHp <= DEVOUR_MAX_HP && (u32)foe->currentHp * 100 <= maxHp * threshold;
}

/** Count kills toward the next charge, gaining charges while below the most the hero holds. */
static void addProgress(u8 playerIndex, SpecState *st, const SpecRebirth *rebirth, Actor *actor, u32 kills)
{
    u32 most = maxCharges(playerIndex, rebirth);
    u32 per = killsPerCharge(playerIndex, rebirth);

    if (st->charges >= most)
    {
        st->progress = 0;
        return;
    }
    kills += st->progress;
    while (kills >= per && st->charges < most)
    {
        kills -= per;
        st->charges++;
        st->flashFrames = METER_FLASH_FRAMES;
        hackLib_fxBurst(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_NAZGUL, FX_LAYER_FRONT);
        hackLib_fxBurst(actor, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    }
    st->progress = st->charges >= most ? 0 : (u8)kills;
}

/** The summon row of the player whose avatar is owner, for allies with a look, or NULL. */
static const SpecSkill *lookRow(Actor *owner)
{
    const SpecSkill *const *rows;

    if (owner == NULL || !skillExpansion_isPlayerAvatar(owner))
    {
        return NULL;
    }
    rows = specRows(owner->as.combat.playerIndex);
    for (u32 row = 0; rows != NULL && row < 13; row++)
    {
        if (rows[row] != NULL && rows[row]->effect == SPEC_EFFECT_SUMMON && rows[row]->lookEnemy != SPEC_LOOK_NONE)
        {
            return rows[row];
        }
    }
    return NULL;
}

/**
 * Draw a ghost ally as an enemy type: whenever the stock ghost puts on one of its own animations,
 * swap in the enemy's for the same facing and action. The enemy's attack frames carry the hitbox.
 */
static void wearLook(Actor *ally, u8 enemyType)
{
    const SpriteAnimation *current = ally->spriteAnimation;
    u8 pose;

    for (u32 k = 0; k < 5; k++)
    {
        const SpriteAnimation *base = NpcAnimationSets[GHOST_NPC_KIND][k];

        if (base != NULL && current >= base && current < base + 5)
        {
            if (ally->behaviorState == GHOST_BEHAVIOR_DESPAWNING || ally->actionState == GHOST_ACTION_DEATH)
            {
                pose = ENEMY_ANIMATION_POSE_DEATH;
            }
            else if (ally->actionState == GHOST_ACTION_ATTACK)
            {
                pose = ENEMY_ANIMATION_POSE_ATTACK;
            }
            else
            {
                pose = ally->actionState == GHOST_ACTION_PURSUIT ? ENEMY_ANIMATION_POSE_WALK
                                                                 : ENEMY_ANIMATION_POSE_STAND;
            }
            if (EnemyAnimationBanks[enemyType][pose] != NULL)
            {
                sprite_setAnimation(ally, EnemyAnimationBanks[enemyType][pose] + (current - base));
            }
            return;
        }
    }
}

static Actor *spawnLookAlly(Actor *owner, Vector2Fp16 *position, Actor *target, u16 lifetime, u16 damage)
{
    Actor *ally = spawnNpc(GHOST_NPC_KIND, position, owner, target, lifetime);
    const SpecSkill *row = lookRow(owner);

    if (ally != NULL)
    {
        ghostState(ally)->damage = damage > 255 ? 255 : damage;
        owner->as.combat.comboState[0]++;
        skillExpansion_trackGhost(ally);
        if (row != NULL)
        {
            hackLib_fxUseFxColor(ally, row->lookColor != 0
                                           ? (HackLibFxColor)row->lookColor
                                           : FX_COLOR_OBJ_PALETTE(ENEMY_PALETTE_IDS[row->lookEnemy]));
            wearLook(ally, row->lookEnemy);
        }
    }
    return ally;
}

static void tickLookAlly(Actor *ally, Actor *target, u16 damage)
{
    const SpecSkill *row = lookRow(ally->parentActor);

    (void)target;
    (void)damage;
    hackLib_tickPersistentGhost(ally, skillExpansion_isFoe, findNextEnemy);
    if (row != NULL)
    {
        wearLook(ally, row->lookEnemy);
    }
}

static void despawnLookAlly(Actor *ally)
{
    ghostState(ally)->lifetime = 1;
}

static bool32 isUsableLookAlly(Actor *ally)
{
    return ghostState(ally)->lifetime > 1;
}

// Persistent ghost allies drawn as another creature (spec summons with a look).
static const HackLibSummonDef LookAllySummon = {
    .spawn = spawnLookAlly,
    .tick = tickLookAlly,
    .despawn = despawnLookAlly,
    .isUsable = isUsableLookAlly,
};

static SkillExpansionCastResult summon(Actor *actor, const SpecSkill *skill, u32 level)
{
    u32 spawned = 0;
    u32 count = skill->count + passiveBonus(actor->as.combat.playerIndex, SPEC_PASSIVE_SUMMON_COUNT);

    count = count > 4 ? 4 : count;
    for (u32 i = 0; i < count; i++)
    {
        Vector2Fp16 preferred = actor->coords;
        Vector2Fp16 landing;
        Actor *ally;
        u32 frames;
        s32 side = (i & 1) ? -1 : 1;

        preferred.x += side * ((s32)(skill->spread != 0 ? skill->spread : SUMMON_SPREAD) << 16);
        preferred.y += (s32)((i >> 1) * 12 - 6) << 16;
        if (!hackLib_findLanding(actor, &preferred, &landing))
        {
            landing = actor->coords;
        }
        frames = skill->frames + 30 * passiveBonus(actor->as.combat.playerIndex, SPEC_PASSIVE_SUMMON_SECONDS);
        ally = skill->lookEnemy != SPEC_LOOK_NONE
                   ? hackLib_spawnSummon(&LookAllySummon, actor, &landing, NULL, (u16)frames,
                                         skill->amount[level - 1], (u8)count, HACKLIB_ABILITY_NONE)
                   : skillExpansion_summonGhost(actor, &landing, NULL, (u16)frames, skill->amount[level - 1],
                                                (u8)count, TRUE);
        if (ally != NULL)
        {
            playFx(skill, actor, ally, NULL, SPEC_PLAY_TARGET);
            spawned++;
        }
    }
    if (spawned != 0)
    {
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    }
    return spawned != 0 ? SKILL_CAST_SUCCESS : SKILL_CAST_INVALID_TARGET;
}

static SkillExpansionCastResult devour(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, (SkillExpansionTargetCondition)devourable,
                                           skill->threshold + passiveBonus(player, SPEC_PASSIVE_DEVOUR_THRESHOLD));
    const SpecRebirth *rebirth = specRebirth(player);

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, foe, NULL, SPEC_PLAY_ALL);
    skillExpansion_dealDamage(actor, foe, 9999, HACKLIB_DAMAGE_SECONDARY, HACKLIB_ABILITY_NONE);
    skillExpansion_original_player_addHp(player, (s16)skill->amount[level - 1], FALSE);
    if (rebirth != NULL)
    {
        addProgress(player, state(player), rebirth, actor, skill->killCredit);
    }
    return SKILL_CAST_SUCCESS;
}

static SkillExpansionCastResult tithe(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    const SpecRebirth *rebirth = specRebirth(player);
    SpecState *st = state(player);
    s32 cost = PLAYER_STAT(player, STAT_MAX_HP) * skill->amount[level - 1] / 100;

    if (rebirth == NULL || st->charges >= maxCharges(player, rebirth) || PLAYER(player).currentHp <= cost)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    skillExpansion_original_player_addHp(player, -cost, FALSE);
    st->charges++;
    st->progress = st->charges >= maxCharges(player, rebirth) ? 0 : st->progress;
    st->flashFrames = METER_FLASH_FRAMES;
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

static void tickCharger(Actor *fx);
static void tickSwooper(Actor *fx);

/** The mover slot of fx, or NULL. */
static SpecMover *findMover(Actor *fx)
{
    for (u32 i = 0; i < MOVER_COUNT; i++)
    {
        if (Movers[i].fx == fx)
        {
            return &Movers[i];
        }
    }
    return NULL;
}

/** A free mover slot for fx, or NULL. A slot whose actor no longer moves is free. */
static SpecMover *claimMover(Actor *fx, const SpecSkill *skill, u8 player)
{
    for (u32 i = 0; i < MOVER_COUNT; i++)
    {
        SpecMover *m = &Movers[i];

        if (m->fx == NULL || (m->fx->tickCallback != tickCharger && m->fx->tickCallback != tickSwooper))
        {
            *m = (SpecMover){.fx = fx, .skill = skill, .player = player};
            return m;
        }
    }
    return NULL;
}

/** Leave the skill's trail layer at the mover every trailTicks. */
static void dropTrail(SpecMover *m, Actor *fx)
{
    const SpecSkill *skill = m->skill;
    Actor *owner = PLAYER(m->player).ownerActor;

    if (skill->trailTicks == 0 || !layerUsed(&skill->trail) || owner == NULL ||
        ++m->age % skill->trailTicks != 0)
    {
        return;
    }
    playLayer(&skill->trail, owner, &fx->coords);
}

/**
 * A charging rider (spec_kit.h SPEC_EFFECT_CHARGE). State in the effect actor: venomDuration is
 * the frames left, storedDamage the hit, playerIndex the caster, comboState the step in pixels per
 * tick (x, y as s8), the reach and the stun in ticks / 2.
 */
static void tickCharger(Actor *fx)
{
    ActorCombatState *c = &fx->as.combat;
    Actor *owner = c->playerIndex < 2 ? PLAYER(c->playerIndex).ownerActor : NULL;
    Vector2Fp16 at = fx->coords;
    Actor *foe = NULL;
    SpecMover *m = findMover(fx);

    if (fx->flags.p & ACTOR_FLAG_ANIMATION_END)
    {
        fx->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
        sprite_setAnimationFrame(fx, 0);
    }
    fx_tickAttachment(fx);
    if (--c->venomDuration <= 0 || owner == NULL)
    {
        if (m != NULL)
        {
            m->fx = NULL;
        }
        hackLib_fxRemove(fx);
        return;
    }
    if (m != NULL)
    {
        dropTrail(m, fx);
    }
    at.x += (s32)(s8)c->comboState[0] << 16;
    at.y += (s32)(s8)c->comboState[1] << 16;
    hackLib_fxDetach(fx, &at);
    while ((foe = hackLib_nextTargetAt(&at, NULL, c->comboState[2], foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->actionState == ACTOR_STATE_STUNNED)
        {
            continue;
        }
        if (!actor_isStunnable(foe) && c->venomDuration % CHARGE_REPEAT_TICKS != 0)
        {
            continue;
        }
        skillExpansion_applyPveRangedHit(owner, foe, c->storedDamage);
        hackLib_applyStun(foe, c->comboState[3] * 2);
        hackLib_fxBurst(foe, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
    }
}

/** A rider charging from `from` along the caster's facing. FALSE when it could not spawn. */
static bool32 spawnRider(Actor *actor, u8 player, const SpecSkill *skill, u32 level, const Vector2Fp16 *from)
{
    u8 facing = actor->facing < 8 ? actor->facing : 2;
    Vector2Fp16 step;
    Actor *fx = hackLib_fxSprite(actor, skill->chargeAnim + FacingArt[facing], (HackLibFxColor)skill->chargeColor,
                                 CHARGE_ANIM_TICKS, FX_LAYER_FRONT);

    if (fx == NULL)
    {
        return FALSE;
    }
    if (facing >= 5)
    {
        fx->oam.matrixIndex |= OAM_MATRIX_H_FLIP;
    }
    actor_setVelocityFromDirection8(&step, actor->flags.p, facing, skill->chargeSpeed << 16);
    hackLib_fxDetach(fx, from);
    fx->as.combat.venomDuration = (s16)skill->frames;
    fx->as.combat.storedDamage = skill->amount[level - 1];
    fx->as.combat.playerIndex = player;
    fx->as.combat.comboState[0] = (u8)(s8)(step.x >> 16);
    fx->as.combat.comboState[1] = (u8)(s8)(step.y >> 16);
    fx->as.combat.comboState[2] = skill->chargeRadius;
    fx->as.combat.comboState[3] = (u8)(skill->stunFrames / 2 > 255 ? 255 : skill->stunFrames / 2);
    fx->tickCallback = tickCharger;
    claimMover(fx, skill, player);
    return TRUE;
}

static SkillExpansionCastResult charge(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    if (skill->chargeAnim == NULL)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    if (!spawnRider(actor, player, skill, level, &actor->coords))
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

/** count riders side by side, starting behind the caster. */
static SkillExpansionCastResult stampede(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    u8 facing = actor->facing < 8 ? actor->facing : 2;
    u32 count = skill->count != 0 ? skill->count : 1;
    u32 spawned = 0;
    Vector2Fp16 unit;

    if (skill->chargeAnim == NULL)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    actor_setVelocityFromDirection8(&unit, actor->flags.p, facing, 1 << 16);
    for (u32 k = 0; k < count; k++)
    {
        s32 side = (s32)(2 * k) - (s32)(count - 1);
        Vector2Fp16 from = actor->coords;

        from.x += -unit.x * STAMPEDE_BACK - unit.y * side * STAMPEDE_SPACING / 2;
        from.y += -unit.y * STAMPEDE_BACK + unit.x * side * STAMPEDE_SPACING / 2;
        spawned += spawnRider(actor, player, skill, level, &from) ? 1 : 0;
    }
    if (spawned == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

/** A struck flyer plays its strike animation once, then despawns. */
static void tickStruck(Actor *fx)
{
    fx_tickAttachment(fx);
}

/** A swoop reaches its strike point: the delivery bursts there, then the flyer strikes. */
static void strike(SpecMover *m, Actor *fx)
{
    const SpecSkill *skill = m->skill;
    Actor *owner = PLAYER(m->player).ownerActor;
    Vector2Fp16 to = m->to;

    m->fx = NULL;
    hackLib_fxDetach(fx, &to);
    if (owner != NULL)
    {
        skillDelivery_launch(m->player, skill->delivery, &to, math_vector2_direction32(m->step), m->level, m->row,
                             FALSE);
        playFx(skill, owner, NULL, &to, SPEC_PLAY_TARGET);
    }
    if (skill->strikeAnim == NULL)
    {
        hackLib_fxRemove(fx);
        return;
    }
    sprite_setAnimation(fx, skill->strikeAnim + FacingArt[2]);
    hackLib_fxPace(fx, STRIKE_ANIM_TICKS);
    fx->tickCallback = tickStruck;
}

/** A flyer diving onto its strike point (SPEC_EFFECT_SWOOP), its state in its mover slot. */
static void tickSwooper(Actor *fx)
{
    SpecMover *m = findMover(fx);
    Vector2Fp16 at = fx->coords;

    if (fx->flags.p & ACTOR_FLAG_ANIMATION_END)
    {
        fx->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
        sprite_setAnimationFrame(fx, 0);
    }
    fx_tickAttachment(fx);
    if (m == NULL)
    {
        hackLib_fxRemove(fx);
        return;
    }
    dropTrail(m, fx);
    at.x += m->step.x;
    at.y += m->step.y;
    hackLib_fxDetach(fx, &at);
    if (--m->left == 0)
    {
        strike(m, fx);
    }
}

static SkillExpansionCastResult swoop(Actor *actor, u8 player, const SpecSkill *skill, u32 level, u8 row)
{
    Actor *target = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);
    u32 frames = skill->frames != 0 ? skill->frames : SWOOP_DEFAULT_FRAMES;
    Vector2Fp16 from;
    SpecMover *m;
    Actor *fx;
    s32 side;

    if (target == NULL || skill->chargeAnim == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    side = target->coords.x >= actor->coords.x ? 1 : -1;
    from = target->coords;
    from.x -= side * (SWOOP_FROM_X << 16);
    from.y -= SWOOP_FROM_Y << 16;
    fx = hackLib_fxSprite(actor, skill->chargeAnim + FacingArt[2], (HackLibFxColor)skill->chargeColor,
                          CHARGE_ANIM_TICKS, FX_LAYER_FRONT);
    m = fx != NULL ? claimMover(fx, skill, player) : NULL;
    if (m == NULL)
    {
        // No flyer to show: strike at once.
        if (fx != NULL)
        {
            hackLib_fxRemove(fx);
        }
        skillDelivery_launch(player, skill->delivery, &target->coords, 0, (u8)level, row, FALSE);
        playFx(skill, actor, target, NULL, SPEC_PLAY_ALL);
        return SKILL_CAST_SUCCESS;
    }
    if (side < 0)
    {
        fx->oam.matrixIndex |= OAM_MATRIX_H_FLIP;
    }
    hackLib_fxDetach(fx, &from);
    m->to = target->coords;
    m->step.x = (m->to.x - from.x) / (s32)frames;
    m->step.y = (m->to.y - from.y) / (s32)frames;
    m->left = (u16)frames;
    m->level = (u8)level;
    m->row = row;
    fx->tickCallback = tickSwooper;
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

typedef enum SpecLastingKind
{
    LASTING_NONE = 0,
    LASTING_ZONE = 1,
    LASTING_ROOT = 2,
    LASTING_FEAR = 3,
    LASTING_CHARM = 4,
    LASTING_MARK = 5,
    // A barrage strike waiting to land on its foe when left runs out.
    LASTING_STRIKE = 6,
    LASTING_WEAKEN = 7,
    // Bleed or venom stacks on a foe.
    LASTING_STACK = 8,
    // A placed trap waiting at `at` (no foe).
    LASTING_TRAP = 9,
    LASTING_PULL = 10,
    LASTING_SLEEP = 11,
    // A kicked foe flying away from the caster.
    LASTING_SHOVE = 12,
    // A foe taking a flurry of cuts (stacks: cuts left).
    LASTING_FLURRY = 13,
    // A blinded foe.
    LASTING_BLIND = 14,
    // A keg waiting at `at` (no foe).
    LASTING_BOMB = 15,
    // A broadside walking out from `at` (no foe, stacks: the facing).
    LASTING_SALVO = 16,
    // A bolt's look flying from `at` by step each tick (no foe).
    LASTING_FLIGHT = 17,
    // A planted shield at `at` (no foe).
    LASTING_COVER = 18,
    // A foe struck by a burning bolt (at: where it was last seen).
    LASTING_IGNITE = 19,
    // A rain of bolts around `at` (no foe).
    LASTING_HAIL = 20,
} SpecLastingKind;

/** Lastings placed at a spot rather than on a foe. */
static bool32 placedKind(u8 kind)
{
    return kind == LASTING_ZONE || kind == LASTING_TRAP || kind == LASTING_BOMB || kind == LASTING_SALVO ||
           kind == LASTING_FLIGHT || kind == LASTING_COVER || kind == LASTING_HAIL;
}

static u32 percentRoll(void);

// A zone, root or fear still working after its cast, ticked by its caster's player.
typedef struct SpecLasting
{
    const SpecSkill *skill;
    // Root and fear: the foe.
    HackLibActorRef foe;
    // Zone: its center unless it follows the caster. Root: where the foe is held.
    Vector2Fp16 at;
    u16 left;
    u16 age;
    // SpecLastingKind, LASTING_NONE for a free slot.
    u8 kind;
    u8 player;
    u8 level;
    // Stack: the stacks on the foe.
    u8 stacks;
    // Trap and sleep: the held web or cocoon sprite.
    HackLibActorRef look;
    // Zone: centered on the caster at every pulse.
    u8 follow;
    // Flight: movement per tick.
    Vector2Fp16 step;
} SpecLasting;

static SpecLasting Lastings[LASTING_COUNT];

static SpecLasting *addLasting(u8 kind, const SpecSkill *skill, u8 player, u32 level, const Vector2Fp16 *at,
                               Actor *foe, bool32 follow)
{
    for (u32 i = 0; i < LASTING_COUNT; i++)
    {
        SpecLasting *l = &Lastings[i];

        if (l->kind == LASTING_NONE)
        {
            *l = (SpecLasting){.skill = skill, .at = *at, .left = skill->frames, .kind = kind, .player = player,
                               .level = (u8)level, .follow = (u8)follow};
            if (foe != NULL)
            {
                l->foe = hackLib_actorRef(foe);
            }
            return l;
        }
    }
    return NULL;
}

/** Hit foe for the skill's amount and status (zones, drains, phases, roots and fears). */
static s32 touch(Actor *owner, Actor *foe, const SpecSkill *skill, u32 level)
{
    s32 damage = skill->amount[level - 1];

    if (damage > 0)
    {
        skillExpansion_applyPveRangedHit(owner, foe, (u16)damage);
    }
    if (skill->status != SKILL_STATUS_NONE && foe->currentHp > 0)
    {
        skillStatus_apply(skill->status, owner, foe, skill->statusFrames, skill->statusPower, HACKLIB_ABILITY_NONE);
    }
    return damage;
}

static void tickZone(SpecLasting *l, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    u32 every = skill->trailTicks != 0 ? skill->trailTicks : ZONE_PULSE_TICKS;
    Vector2Fp16 center = l->follow ? owner->coords : l->at;
    Actor *foe = NULL;
    s32 dealt = 0;
    u32 hit = 0;

    if (l->age % every != 0)
    {
        return;
    }
    if (layerUsed(&skill->trail))
    {
        playLayer(&skill->trail, owner, &center);
    }
    while ((foe = hackLib_nextTargetAt(&center, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && foe->currentHp > 0)
        {
            dealt += touch(owner, foe, skill, l->level);
            if (hit++ < LOOK_FOES && foe->currentHp > 0)
            {
                playFx(skill, owner, foe, NULL, SPEC_PLAY_TARGET);
            }
        }
    }
    if (skill->percent != 0 && dealt > 0)
    {
        s32 heal = dealt * skill->percent / 100;

        player_addHp(l->player, (s16)(heal < 1 ? 1 : heal), FALSE);
    }
}

/** A rooted foe stays where it was rooted. */
static void tickRoot(SpecLasting *l, Actor *foe)
{
    foe->renderCoords.x += l->at.x - foe->coords.x;
    foe->renderCoords.y += l->at.y - foe->coords.y;
    foe->coords = l->at;
    foe->velocity.x = foe->velocity.y = 0;
}

/** A frightened foe stays stunned and is pushed away from the caster. */
static void tickFear(SpecLasting *l, Actor *foe, Actor *owner)
{
    s32 away = actor_direction8Toward(owner, foe);
    Vector2Fp16 step;
    Vector2Fp16 next = foe->coords;

    if (foe->actionState != ACTOR_STATE_STUNNED)
    {
        hackLib_applyStun(foe, l->left);
    }
    actor_setVelocityFromDirection8(&step, foe->flags.p, away, (l->skill->chargeSpeed != 0 ? l->skill->chargeSpeed : 1)
                                                                    << 16);
    next.x += step.x;
    next.y += step.y;
    if (hackLib_canLandAt(foe, &next))
    {
        foe->renderCoords.x += step.x;
        foe->renderCoords.y += step.y;
        foe->coords = next;
    }
    foe->facing = (Direction8)away;
}

/** A barrage strike lands: the skill's hit, and its trail as the impact look. */
static void strikeFoe(SpecLasting *l, Actor *foe, Actor *owner)
{
    touch(owner, foe, l->skill, l->level);
    if (layerUsed(&l->skill->trail))
    {
        playLayer(&l->skill->trail, owner, &foe->coords);
    }
}

/** The other foe nearest to foe within radius, or NULL. */
static Actor *nearestOtherFoe(Actor *foe, u16 radius)
{
    Actor *best = NULL;
    Actor *other = NULL;
    s32 bestDistance = 0;

    while ((other = hackLib_nextTargetAt(&foe->coords, foe, radius, other)) != NULL)
    {
        s32 dx = (other->coords.x - foe->coords.x) >> 16;
        s32 dy = (other->coords.y - foe->coords.y) >> 16;
        s32 distance = dx * dx + dy * dy;

        if (skillExpansion_isFoe(other) && other->currentHp > 0 && (best == NULL || distance < bestDistance))
        {
            best = other;
            bestDistance = distance;
        }
    }
    return best;
}

/**
 * A charmed foe stays stunned (it never attacks the caster), walks to the nearest other foe and
 * strikes it for the skill's amount every CHARM_STRIKE_TICKS.
 */
static void tickCharm(SpecLasting *l, Actor *foe, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    Actor *victim = nearestOtherFoe(foe, skill->chargeRadius);
    s32 toward;
    s32 dx;
    s32 dy;

    if (foe->actionState != ACTOR_STATE_STUNNED)
    {
        hackLib_applyStun(foe, l->left);
    }
    if (victim == NULL)
    {
        return;
    }
    toward = actor_direction8Toward(foe, victim);
    foe->facing = (Direction8)toward;
    dx = (victim->coords.x - foe->coords.x) >> 16;
    dy = (victim->coords.y - foe->coords.y) >> 16;
    if (dx * dx + dy * dy > CHARM_REACH * CHARM_REACH)
    {
        Vector2Fp16 step;
        Vector2Fp16 next = foe->coords;

        actor_setVelocityFromDirection8(&step, foe->flags.p, toward,
                                        (skill->chargeSpeed != 0 ? skill->chargeSpeed : 1) << 16);
        next.x += step.x;
        next.y += step.y;
        if (hackLib_canLandAt(foe, &next))
        {
            foe->renderCoords.x += step.x;
            foe->renderCoords.y += step.y;
            foe->coords = next;
        }
        return;
    }
    if (l->age % CHARM_STRIKE_TICKS == 0)
    {
        touch(owner, victim, skill, l->level);
        if (layerUsed(&skill->trail))
        {
            playLayer(&skill->trail, owner, &victim->coords);
        }
    }
}

/** Whether a player's mark is on foe, and the strongest mark's percent. */
static u32 markPercent(Actor *foe)
{
    u32 percent = 0;

    for (u32 i = 0; i < LASTING_COUNT; i++)
    {
        const SpecLasting *l = &Lastings[i];

        if (l->kind == LASTING_MARK && hackLib_isActorRefValid(l->foe) && l->foe.actor == foe &&
            l->skill->percent > percent)
        {
            percent = l->skill->percent;
        }
    }
    return percent;
}

/** A layer's sprite held for frames (an effect's first piece, an anim or a looping shape). */
static HackLibActorRef spawnLook(const SpecFx *fx, Actor *on, const Vector2Fp16 *at, u16 frames)
{
    Actor *look = NULL;

    if (fx->effect != 0)
    {
        HackLibEffectHandle handle = {0};

        hackLib_playEffect(fx->effect - 1, on, at, &handle);
        look = hackLib_isActorRefValid(handle.actors[0]) ? handle.actors[0].actor : NULL;
    }
    else if (fx->anim != NULL)
    {
        look = hackLib_fxSprite(on, fx->anim, (HackLibFxColor)fx->color, fx->ticks != 0 ? fx->ticks : ANIM_FX_TICKS,
                                fx->layer);
    }
    else if (fx->color != 0)
    {
        look = hackLib_fxTimed(on, fx->shape, (HackLibFxColor)fx->color, frames, fx->layer);
    }
    if (look == NULL)
    {
        return (HackLibActorRef){0};
    }
    if (at != NULL && fx->effect == 0)
    {
        hackLib_fxDetach(look, at);
    }
    if (fx->effect != 0 || fx->anim != NULL)
    {
        hackLib_fxHold(look, frames);
    }
    return hackLib_actorRef(look);
}

/** Free a lasting: its held sprite goes, a sleeper wakes and a dragged foe is let go. */
static void endLasting(SpecLasting *l, Actor *foe)
{
    if (hackLib_isActorRefValid(l->look))
    {
        hackLib_fxRemove(l->look.actor);
    }
    if ((l->kind == LASTING_SLEEP || l->kind == LASTING_PULL) && foe != NULL &&
        foe->actionState == ACTOR_STATE_STUNNED)
    {
        foe->actionStateTimer = 1;
    }
    l->kind = LASTING_NONE;
}

/** A trap springs on the first foes within its reach: each is held, hit and shows the target look. */
static void tickTrap(SpecLasting *l, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    Actor *foe = NULL;
    u32 caught = 0;

    while ((foe = hackLib_nextTargetAt(&l->at, NULL, skill->chargeRadius, foe)) != NULL)
    {
        SpecLasting *held;

        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        touch(owner, foe, skill, l->level);
        held = addLasting(LASTING_ROOT, skill, l->player, l->level, &foe->coords, foe, FALSE);
        if (held != NULL)
        {
            held->left = skill->stunFrames;
        }
        if (caught < LOOK_FOES)
        {
            playFx(skill, owner, foe, NULL, SPEC_PLAY_TARGET);
        }
        caught++;
    }
    if (caught != 0)
    {
        endLasting(l, NULL);
    }
}

/** Weaken, stack, pull and sleep: what each does every tick. */
static void tickHunt(SpecLasting *l, Actor *foe, Actor *owner)
{
    const SpecSkill *skill = l->skill;

    if (l->kind == LASTING_STACK)
    {
        if (l->age % STACK_TICKS == 0 && l->stacks != 0)
        {
            skillExpansion_applyPveRangedHit(owner, foe, (u16)(skill->amount[l->level - 1] * l->stacks));
            if (skill->chargeColor != 0)
            {
                hackLib_fxBurst(foe, FX_SHAPE_SPARK, (HackLibFxColor)skill->chargeColor, FX_LAYER_FRONT);
            }
        }
    }
    else if (l->kind == LASTING_SLEEP)
    {
        if (foe->actionState != ACTOR_STATE_STUNNED)
        {
            hackLib_applyStun(foe, l->left);
        }
    }
    else if (l->kind == LASTING_PULL)
    {
        s32 dx = (owner->coords.x - foe->coords.x) >> 16;
        s32 dy = (owner->coords.y - foe->coords.y) >> 16;
        Vector2Fp16 step;
        Vector2Fp16 next = foe->coords;

        if (dx * dx + dy * dy <= PULL_REACH * PULL_REACH)
        {
            endLasting(l, foe);
            return;
        }
        if (foe->actionState != ACTOR_STATE_STUNNED)
        {
            hackLib_applyStun(foe, l->left);
        }
        actor_setVelocityFromDirection8(&step, foe->flags.p, actor_direction8Toward(foe, owner),
                                        (skill->chargeSpeed != 0 ? skill->chargeSpeed : 4) << 16);
        next.x += step.x;
        next.y += step.y;
        if (hackLib_canLandAt(foe, &next))
        {
            foe->renderCoords.x += step.x;
            foe->renderCoords.y += step.y;
            foe->coords = next;
        }
        if (skill->trailTicks != 0 && l->age % skill->trailTicks == 0 && layerUsed(&skill->trail))
        {
            playLayer(&skill->trail, owner, &foe->coords);
        }
    }
}

/** The strongest weaken on foe, in percent less damage. */
static u32 weakenPercent(Actor *foe)
{
    u32 percent = 0;

    for (u32 i = 0; i < LASTING_COUNT; i++)
    {
        const SpecLasting *l = &Lastings[i];

        if (l->kind == LASTING_WEAKEN && hackLib_isActorRefValid(l->foe) && l->foe.actor == foe &&
            l->skill->percent > percent)
        {
            percent = l->skill->percent;
        }
    }
    return percent;
}

/** A powder blast at a spot: every foe in reach is hit and stunned, the target layers play there. */
static void blastAt(Actor *owner, const SpecSkill *skill, u32 level, const Vector2Fp16 *at)
{
    Actor *foe = NULL;

    while ((foe = hackLib_nextTargetAt(at, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && foe->currentHp > 0)
        {
            touch(owner, foe, skill, level);
            if (foe->currentHp > 0)
            {
                hackLib_applyStun(foe, skill->stunFrames);
            }
        }
    }
    playFx(skill, owner, NULL, at, SPEC_PLAY_TARGET);
}

/** A bolt of a rain drops on a random spot near the rain's center. */
static void tickHail(SpecLasting *l, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    u32 every = skill->trailTicks != 0 ? skill->trailTicks : 8;
    s32 r = skill->chargeRadius;
    Vector2Fp16 at = l->at;
    Actor *foe = NULL;

    if (l->age % every != 0)
    {
        return;
    }
    at.x += ((s32)percentRoll() * 2 - 100) * r / 100 << 16;
    at.y += ((s32)percentRoll() * 2 - 100) * r * 6 / 1000 << 16;
    while ((foe = hackLib_nextTargetAt(&at, NULL, HAIL_HIT_RADIUS, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && foe->currentHp > 0)
        {
            touch(owner, foe, skill, l->level);
            if (foe->currentHp > 0)
            {
                hackLib_applyStun(foe, skill->stunFrames);
            }
        }
    }
    playFx(skill, owner, NULL, &at, SPEC_PLAY_TARGET);
}

/** A burning foe died: the fire leaps to every foe near where it fell. */
static void igniteSpread(SpecLasting *l, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    Actor *foe = NULL;
    u32 caught = 0;

    while ((foe = hackLib_nextTargetAt(&l->at, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        touch(owner, foe, skill, l->level);
        if (caught++ < LOOK_FOES)
        {
            playFx(skill, owner, foe, NULL, SPEC_PLAY_TARGET);
        }
    }
    playFx(skill, owner, NULL, &l->at, SPEC_PLAY_TARGET);
}

/** A broadside's next blast lands spread pixels further along its facing. */
static void tickSalvo(SpecLasting *l, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    u32 every = skill->trailTicks != 0 ? skill->trailTicks : 6;
    Vector2Fp16 unit;
    Vector2Fp16 at = l->at;
    s32 step;

    if (l->age % every != 0)
    {
        return;
    }
    step = (s32)(l->age / every) * (skill->spread != 0 ? skill->spread : 32);
    actor_setVelocityFromDirection8(&unit, owner->flags.p, l->stacks, 1 << 16);
    at.x += unit.x * step;
    at.y += unit.y * step;
    blastAt(owner, skill, l->level, &at);
}

/** A foe under a flurry is held and cut every trailTicks until the cuts run out. */
static void tickFlurry(SpecLasting *l, Actor *foe, Actor *owner)
{
    const SpecSkill *skill = l->skill;
    u32 every = skill->trailTicks != 0 ? skill->trailTicks : 6;

    if (foe->actionState != ACTOR_STATE_STUNNED)
    {
        hackLib_applyStun(foe, every + 2);
    }
    if (l->age % every != 0)
    {
        return;
    }
    touch(owner, foe, skill, l->level);
    if (layerUsed(&skill->trail))
    {
        playLayer(&skill->trail, owner, &foe->coords);
    }
    if (l->stacks == 0 || --l->stacks == 0)
    {
        endLasting(l, foe);
    }
}

static void tickLastings(u8 player, Actor *owner)
{
    for (u32 i = 0; i < LASTING_COUNT; i++)
    {
        SpecLasting *l = &Lastings[i];
        Actor *foe;

        if (l->kind == LASTING_NONE || l->player != player)
        {
            continue;
        }
        foe = !placedKind(l->kind) && hackLib_isActorRefValid(l->foe) ? l->foe.actor : NULL;
        if (l->left == 0 || --l->left == 0)
        {
            if (l->kind == LASTING_STRIKE && foe != NULL && foe->currentHp > 0)
            {
                strikeFoe(l, foe, owner);
            }
            if (l->kind == LASTING_BOMB)
            {
                blastAt(owner, l->skill, l->level, &l->at);
            }
            endLasting(l, foe);
            continue;
        }
        l->age++;
        if (l->kind == LASTING_ZONE)
        {
            tickZone(l, owner);
            continue;
        }
        if (l->kind == LASTING_TRAP)
        {
            tickTrap(l, owner);
            continue;
        }
        if (l->kind == LASTING_SALVO)
        {
            tickSalvo(l, owner);
            continue;
        }
        if (l->kind == LASTING_BOMB || l->kind == LASTING_COVER)
        {
            continue;
        }
        if (l->kind == LASTING_FLIGHT)
        {
            l->at.x += l->step.x;
            l->at.y += l->step.y;
            playLayer(&l->skill->trail, owner, &l->at);
            continue;
        }
        if (l->kind == LASTING_HAIL)
        {
            tickHail(l, owner);
            continue;
        }
        if (l->kind == LASTING_IGNITE && (foe == NULL || foe->currentHp <= 0))
        {
            igniteSpread(l, owner);
            endLasting(l, NULL);
            continue;
        }
        if (foe == NULL || foe->currentHp <= 0)
        {
            endLasting(l, NULL);
            continue;
        }
        if (l->kind == LASTING_SHOVE)
        {
            tickFear(l, foe, owner);
            continue;
        }
        if (l->kind == LASTING_FLURRY)
        {
            tickFlurry(l, foe, owner);
            continue;
        }
        if (l->kind == LASTING_BLIND)
        {
            continue;
        }
        if (l->kind == LASTING_IGNITE)
        {
            l->at = foe->coords;
            continue;
        }
        if (l->kind >= LASTING_WEAKEN)
        {
            tickHunt(l, foe, owner);
            continue;
        }
        if (l->kind == LASTING_ROOT)
        {
            tickRoot(l, foe);
        }
        else if (l->kind == LASTING_FEAR)
        {
            tickFear(l, foe, owner);
        }
        else if (l->kind == LASTING_CHARM)
        {
            tickCharm(l, foe, owner);
        }
    }
}

/** Root or frighten every foe within reach. */
static SkillExpansionCastResult afflict(Actor *actor, u8 player, const SpecSkill *skill, u32 level, u8 kind)
{
    Actor *foe = NULL;
    u32 hit = 0;

    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        touch(actor, foe, skill, level);
        if (foe->currentHp <= 0 || (kind == LASTING_FEAR && !hackLib_applyStun(foe, skill->frames) &&
                                    foe->actionState != ACTOR_STATE_STUNNED))
        {
            continue;
        }
        if (addLasting(kind, skill, player, level, &foe->coords, foe, FALSE) == NULL)
        {
            break;
        }
        if (hit < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
        hit++;
    }
    if (hit == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

static SkillExpansionCastResult zone(Actor *actor, u8 player, const SpecSkill *skill, u32 level, bool32 follow)
{
    if (addLasting(LASTING_ZONE, skill, player, level, &actor->coords, NULL, follow) == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

static void grantSpirit(u8 player, s32 amount)
{
    s32 most = PLAYER_STAT(player, STAT_MAX_SPIRIT);
    s32 spirit = PLAYER(player).currentSpirit + amount;

    PLAYER(player).currentSpirit = (s16)(spirit > most ? most : spirit);
    g_PlayerHuds[player].flags.p |= PLAYERHUD_FLAG_SPIRIT_DIRTY;
}

/** The nearest foe within targetRange fights for the caster. Bosses (unstunnable) refuse. */
static SkillExpansionCastResult charm(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);

    if (foe == NULL || !actor_isStunnable(foe) ||
        addLasting(LASTING_CHARM, skill, player, level, &foe->coords, foe, FALSE) == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    hackLib_applyStun(foe, skill->frames);
    playFx(skill, actor, foe, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

/** Foes around take the skill's hit, and the caster gains percent Spirit for each. */
static SkillExpansionCastResult dismay(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Actor *foe = NULL;
    u32 hit = 0;

    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        touch(actor, foe, skill, level);
        if (hit < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
        hit++;
    }
    if (hit == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    grantSpirit(player, (s32)(hit * skill->percent));
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** Up to count foes within targetRange are marked now and struck frames later. */
static SkillExpansionCastResult barrage(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Actor *foe = NULL;
    u32 marked = 0;
    u32 most = skill->count != 0 ? skill->count : 1;

    while (marked < most && (foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->targetRange, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0 ||
            addLasting(LASTING_STRIKE, skill, player, level, &foe->coords, foe, FALSE) == NULL)
        {
            continue;
        }
        playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        marked++;
    }
    if (marked == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** A dashing caster rushes on along his facing, trampling foes (SPEC_EFFECT_DASH). */
static void tickDash(u8 player, SpecState *st, Actor *actor)
{
    const SpecSkill *const *rows = specRows(player);
    const SpecSkill *skill = st->dashRow < 13 && rows != NULL ? rows[st->dashRow] : NULL;
    u8 facing = actor->facing < 8 ? actor->facing : 2;
    Vector2Fp16 step;
    Vector2Fp16 next = actor->coords;
    Actor *foe = NULL;

    if (--st->dashFrames == 0 || skill == NULL || st->dashLevel == 0)
    {
        st->dashFrames = 0;
        return;
    }
    actor_setVelocityFromDirection8(&step, actor->flags.p, st->dashBack ? (facing + 4) % 8 : facing,
                                    skill->chargeSpeed << 16);
    next.x += step.x;
    next.y += step.y;
    if (!hackLib_canLandAt(actor, &next))
    {
        st->dashFrames = 0;
        return;
    }
    actor->renderCoords.x += step.x;
    actor->renderCoords.y += step.y;
    actor->coords = next;
    if (st->dashBack)
    {
        return;
    }
    if (skill->trailTicks != 0 && st->dashFrames % skill->trailTicks == 0 && layerUsed(&skill->trail))
    {
        playLayer(&skill->trail, actor, &actor->coords);
    }
    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0 || foe->actionState == ACTOR_STATE_STUNNED ||
            (!actor_isStunnable(foe) && st->dashFrames % CHARGE_REPEAT_TICKS != 0))
        {
            continue;
        }
        touch(actor, foe, skill, st->dashLevel);
        hackLib_applyStun(foe, skill->stunFrames);
        hackLib_fxBurst(foe, FX_SHAPE_STAR_BLOOM, FX_COLOR_WHITE, FX_LAYER_FRONT);
    }
}

// Per player: a leap in flight, bites that stack, and Frenzy.
typedef struct SpecHunt
{
    // Leap: the foe leapt at and the step per tick.
    HackLibActorRef target;
    Vector2Fp16 step;
    // Leap: ticks left, 0 when none.
    u16 leapFrames;
    u8 leapRow;
    u8 leapLevel;
    // Leap: leaps left after this one.
    u8 chainLeft;
    // Backstep (a dash backwards that hits nothing).
    u8 backstep;
    // Bleed or venom: frames left, the row and level.
    u16 onHitFrames;
    u8 onHitRow;
    u8 onHitLevel;
    // Frenzy: stacks, the raised cap (0: the spec's), frames left of the raise, ticks since a hit.
    u8 stacks;
    u8 cap;
    u16 capFrames;
    u16 idleFrames;
} SpecHunt;

static SpecHunt Hunts[2];

static const SpecFrenzy *specFrenzy(u8 playerIndex)
{
    const SpecKitTable *table = specTable(playerIndex);

    return table != NULL ? table->frenzy : NULL;
}

static u32 frenzyCap(u8 player, const SpecFrenzy *frenzy)
{
    return Hunts[player].cap != 0 ? Hunts[player].cap : frenzy->maxStacks;
}

/** Start a leap from actor to foe. */
static void leapAt(SpecHunt *h, const SpecSkill *skill, Actor *actor, Actor *foe)
{
    s32 dx = (foe->coords.x - actor->coords.x) >> 16;
    s32 dy = (foe->coords.y - actor->coords.y) >> 16;
    s32 distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
    s32 frames = distance / (skill->chargeSpeed != 0 ? skill->chargeSpeed : 6);

    frames = frames < 4 ? 4 : frames > 30 ? 30 : frames;
    h->target = hackLib_actorRef(foe);
    h->step.x = (foe->coords.x - actor->coords.x) / frames;
    h->step.y = (foe->coords.y - actor->coords.y) / frames;
    h->leapFrames = (u16)frames;
    actor->facing = (Direction8)actor_direction8Toward(actor, foe);
}

/** A leaping caster flies on, bites on landing, then leaps at the next foe while the chain lasts. */
static void tickLeap(u8 player, Actor *actor)
{
    SpecHunt *h = &Hunts[player];
    const SpecSkill *const *rows = specRows(player);
    const SpecSkill *skill = h->leapRow < 13 && rows != NULL ? rows[h->leapRow] : NULL;
    Actor *foe = hackLib_isActorRefValid(h->target) ? h->target.actor : NULL;
    Vector2Fp16 next = actor->coords;
    s32 dx;
    s32 dy;

    if (skill == NULL || h->leapLevel == 0 || foe == NULL || foe->currentHp <= 0)
    {
        h->leapFrames = 0;
        return;
    }
    next.x += h->step.x;
    next.y += h->step.y;
    if (hackLib_canLandAt(actor, &next))
    {
        actor->renderCoords.x += h->step.x;
        actor->renderCoords.y += h->step.y;
        actor->coords = next;
    }
    dx = (foe->coords.x - actor->coords.x) >> 16;
    dy = (foe->coords.y - actor->coords.y) >> 16;
    if (--h->leapFrames != 0 && dx * dx + dy * dy > LEAP_REACH * LEAP_REACH)
    {
        return;
    }
    h->leapFrames = 0;
    touch(actor, foe, skill, h->leapLevel);
    hackLib_applyStun(foe, skill->stunFrames);
    playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
    if (h->chainLeft != 0)
    {
        Actor *other = nearestOtherFoe(foe, skill->targetRange);

        h->chainLeft--;
        if (other != NULL)
        {
            leapAt(h, skill, actor, other);
        }
    }
}

/** A bite while bleed or venom runs adds a stack to victim. Venom paralyzes at count stacks. */
static void addStack(u8 player, Actor *owner, Actor *victim)
{
    SpecHunt *h = &Hunts[player];
    const SpecSkill *const *rows = specRows(player);
    const SpecSkill *skill = h->onHitRow < 13 && rows != NULL ? rows[h->onHitRow] : NULL;
    SpecLasting *l = NULL;

    if (skill == NULL || h->onHitLevel == 0 || victim == NULL || victim->currentHp <= 0)
    {
        return;
    }
    for (u32 i = 0; i < LASTING_COUNT && l == NULL; i++)
    {
        if (Lastings[i].kind == LASTING_STACK && Lastings[i].skill == skill &&
            hackLib_isActorRefValid(Lastings[i].foe) && Lastings[i].foe.actor == victim)
        {
            l = &Lastings[i];
        }
    }
    if (l == NULL)
    {
        l = addLasting(LASTING_STACK, skill, player, h->onHitLevel, &victim->coords, victim, FALSE);
        if (l == NULL)
        {
            return;
        }
        l->stacks = 0;
    }
    l->left = skill->statusFrames != 0 ? skill->statusFrames : 90;
    l->stacks = l->stacks < STACK_MAX ? l->stacks + 1 : STACK_MAX;
    if (skill->count != 0 && l->stacks >= skill->count)
    {
        l->stacks = 0;
        hackLib_applyStun(victim, skill->stunFrames);
        if (layerUsed(&skill->trail))
        {
            playLayer(&skill->trail, owner, &victim->coords);
        }
    }
}

static SkillExpansionCastResult leap(Actor *actor, u8 player, const SpecSkill *skill, u32 level, u8 row)
{
    SpecHunt *h = &Hunts[player];
    Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    h->leapRow = row;
    h->leapLevel = (u8)level;
    h->chainLeft = skill->count;
    leapAt(h, skill, actor, foe);
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** The most wounded foe within targetRange takes amount, triple below percent HP. */
static SkillExpansionCastResult finisher(Actor *actor, const SpecSkill *skill, u32 level)
{
    Actor *best = NULL;
    Actor *foe = NULL;
    s32 bestPercent = 101;
    s32 damage = skill->amount[level - 1];

    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->targetRange, foe)) != NULL)
    {
        s32 most;
        s32 percent;

        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        most = combat_getEnemyMaxHp(foe->unitType);
        percent = most > 0 ? foe->currentHp * 100 / most : 100;
        if (percent < bestPercent)
        {
            best = foe;
            bestPercent = percent;
        }
    }
    if (best == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    if (bestPercent < skill->percent)
    {
        damage *= 3;
    }
    actor->facing = (Direction8)actor_direction8Toward(actor, best);
    skillExpansion_applyPveRangedHit(actor, best, (u16)(damage > 0xffff ? 0xffff : damage));
    playFx(skill, actor, best, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

/** A trap at `at` showing the skill's trail until it springs. */
static void placeTrap(Actor *actor, u8 player, const SpecSkill *skill, u32 level, const Vector2Fp16 *at)
{
    SpecLasting *l = addLasting(LASTING_TRAP, skill, player, level, at, NULL, FALSE);

    if (l != NULL && layerUsed(&skill->trail))
    {
        l->look = spawnLook(&skill->trail, actor, at, skill->frames);
    }
}

/** count traps: one at the caster, or a ring spread pixels around him. */
static SkillExpansionCastResult traps(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    u32 count = skill->count != 0 ? skill->count : 1;

    if (count == 1)
    {
        placeTrap(actor, player, skill, level, &actor->coords);
    }
    for (u32 k = 0; count > 1 && k < count; k++)
    {
        Vector2Fp16 offset;
        Vector2Fp16 at = actor->coords;

        actor_setVelocityFromDirection8(&offset, actor->flags.p, (s32)(k * 8 / count), skill->spread << 16);
        at.x += offset.x;
        at.y += offset.y;
        placeTrap(actor, player, skill, level, &at);
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** The nearest stunnable foe within targetRange is dragged in or put to sleep. */
static SkillExpansionCastResult seize(Actor *actor, u8 player, const SpecSkill *skill, u32 level, u8 kind)
{
    Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);
    SpecLasting *l;

    if (foe == NULL || !actor_isStunnable(foe) ||
        (l = addLasting(kind, skill, player, level, &foe->coords, foe, FALSE)) == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    hackLib_applyStun(foe, skill->frames);
    if (kind == LASTING_SLEEP && layerUsed(&skill->trail))
    {
        // Placed at the sleeper's feet: an attached look ends with the foe's attachment tick.
        l->look = spawnLook(&skill->trail, foe, &foe->coords, skill->frames);
    }
    playFx(skill, actor, foe, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

/** Bleed and venom windows run out, a raised Frenzy cap drops back, idle Frenzy stacks drain. */
static void tickHuntTimers(u8 player)
{
    SpecHunt *h = &Hunts[player];
    const SpecFrenzy *frenzy = specFrenzy(player);

    if (h->onHitFrames != 0)
    {
        h->onHitFrames--;
    }
    if (frenzy == NULL)
    {
        return;
    }
    if (h->capFrames != 0 && --h->capFrames == 0)
    {
        h->cap = 0;
    }
    if (h->stacks > frenzyCap(player, frenzy))
    {
        h->stacks = (u8)frenzyCap(player, frenzy);
    }
    if (h->idleFrames < 0xffff)
    {
        h->idleFrames++;
    }
    if (h->stacks != 0 && h->idleFrames >= frenzy->decayFrames && h->idleFrames % FRENZY_DRAIN_TICKS == 0)
    {
        h->stacks--;
    }
}

/** An intangible caster chills the foes around him (SPEC_EFFECT_PHASE). */
static void tickPhase(u8 player, SpecState *st, Actor *actor)
{
    const SpecSkill *const *rows = specRows(player);
    const SpecSkill *skill = st->phaseRow < 13 && rows != NULL ? rows[st->phaseRow] : NULL;
    Actor *foe = NULL;

    if (--st->phaseFrames == 0 || skill == NULL || st->phaseLevel == 0)
    {
        st->phaseFrames = 0;
        actor->spriteFlags &= ~SPRITE_FLAG_STEALTH;
        return;
    }
    actor->spriteFlags |= SPRITE_FLAG_STEALTH;
    if (st->phaseFrames % PHASE_TOUCH_TICKS != 0)
    {
        return;
    }
    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && foe->currentHp > 0)
        {
            touch(actor, foe, skill, st->phaseLevel);
            if (layerUsed(&skill->trail))
            {
                playLayer(&skill->trail, actor, &foe->coords);
            }
        }
    }
}

/** The caster's summoned allies stay frames longer. */
static void rally(Actor *actor, u16 frames)
{
    Actor *ally = NULL;

    while ((ally = hackLib_nextOwnedGhost(actor, ally)) != NULL)
    {
        u32 lifetime = ghostState(ally)->lifetime + frames;

        ghostState(ally)->lifetime = lifetime > 0xfff0 ? 0xfff0 : (u16)lifetime;
        hackLib_fxBurst(ally, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
    }
}

// A timed effect on the caster: frames left, its row and level.
typedef struct SpecTimed
{
    u16 frames;
    u8 row;
    u8 level;
} SpecTimed;

// Ticks between a dread's pulses when its spec names none.
#define DREAD_PULSE_TICKS 30

// Per player: a haste and a cleave running, and the cleave hit waiting for the next tick.
typedef struct SpecCommand
{
    SpecTimed reflect;
    SpecTimed dread;
    SpecTimed undying;
    SpecTimed absorb;
    SpecTimed lifesteal;
    SpecTimed rampart;
    SpecTimed madness;
    SpecTimed plunder;
    SpecTimed aim;
    // Aim: ticks the caster has stood still, and where he stood last tick.
    u16 stillTicks;
    Vector2Fp16 aimCoords;
    // Absorb: damage still to soak.
    u16 absorbLeft;
    // Lifesteal: HP to heal on the next tick.
    u16 pendingHeal;
    // Reflect: the foe that struck last and the share it takes back on the next tick, 0 for none.
    HackLibActorRef reflectFoe;
    u16 reflectDamage;
    // Haste: frames left, row, level and where the caster stood last tick.
    u16 hasteFrames;
    u8 hasteRow;
    u8 hasteLevel;
    Vector2Fp16 lastCoords;
    // Cleave: frames left, row and level.
    u16 cleaveFrames;
    u8 cleaveRow;
    u8 cleaveLevel;
    // Cleave: the foe a melee hit just struck and the share the others take, 0 for none.
    HackLibActorRef cleaveVictim;
    u16 cleaveDamage;
} SpecCommand;

static SpecCommand Commands[2];

/** The caster's facing as a unit step (fixed point). */
static Vector2Fp16 facingUnit(Actor *actor)
{
    Vector2Fp16 unit;

    actor_setVelocityFromDirection8(&unit, actor->flags.p, actor->facing < 8 ? actor->facing : 2, 1 << 16);
    return unit;
}

/** Where foe lies from the caster along unit: pixels ahead (negative behind) and pixels to the side. */
static void offsetAlong(Actor *actor, Actor *foe, Vector2Fp16 unit, s32 *ahead, s32 *aside)
{
    s32 dx = (foe->coords.x - actor->coords.x) >> 16;
    s32 dy = (foe->coords.y - actor->coords.y) >> 16;
    s32 side = (dy * unit.x - dx * unit.y) >> 16;

    *ahead = (dx * unit.x + dy * unit.y) >> 16;
    *aside = side < 0 ? -side : side;
}

/** Every foe on a line ahead of the caster is struck and staggered. The crack shows even on a miss. */
static SkillExpansionCastResult lash(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Vector2Fp16 unit = facingUnit(actor);
    Actor *foe = NULL;
    u32 hit = 0;

    (void)player;
    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->targetRange, foe)) != NULL)
    {
        s32 ahead;
        s32 aside;

        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        offsetAlong(actor, foe, unit, &ahead, &aside);
        if (ahead < 0 || ahead > skill->targetRange || aside > skill->chargeRadius)
        {
            continue;
        }
        touch(actor, foe, skill, level);
        hackLib_applyStun(foe, skill->stunFrames);
        if (hit < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
        hit++;
    }
    for (s32 d = LASH_STEP; layerUsed(&skill->trail) && d <= skill->targetRange; d += LASH_STEP)
    {
        Vector2Fp16 at = actor->coords;

        at.x += unit.x * d;
        at.y += unit.y * d;
        playLayer(&skill->trail, actor, &at);
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** Foes in a cone ahead are kicked away and left lying. Bosses only take the hit. */
static SkillExpansionCastResult shove(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Vector2Fp16 unit = facingUnit(actor);
    Actor *foe = NULL;
    u32 hit = 0;

    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        s32 ahead;
        s32 aside;
        SpecLasting *l;

        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        offsetAlong(actor, foe, unit, &ahead, &aside);
        if (ahead < 0 || aside > ahead + 8)
        {
            continue;
        }
        touch(actor, foe, skill, level);
        if (foe->currentHp > 0 && actor_isStunnable(foe) && hackLib_applyStun(foe, SHOVE_FRAMES + skill->stunFrames) &&
            (l = addLasting(LASTING_SHOVE, skill, player, level, &foe->coords, foe, FALSE)) != NULL)
        {
            l->left = SHOVE_FRAMES;
        }
        if (hit < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
        hit++;
    }
    if (hit == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** Foes around flinch and turn on the caster, who hardens for each of them. */
static SkillExpansionCastResult taunt(Actor *actor, SpecState *st, const SpecSkill *skill, u32 level)
{
    Actor *foe = NULL;
    u32 count = 0;
    u32 percent;

    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        hackLib_applyStun(foe, skill->stunFrames);
        foe->facing = (Direction8)actor_direction8Toward(foe, actor);
        if (count < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
        count++;
    }
    if (count == 0)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    percent = count * skill->amount[level - 1];
    st->wardPercent = (u8)(percent > skill->percent ? skill->percent : percent);
    st->wardFrames = skill->frames;
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
    return SKILL_CAST_SUCCESS;
}

/** A healing that grows with the foes around the caster. */
static SkillExpansionCastResult secondWind(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Actor *foe = NULL;
    u32 count = 0;

    if (PLAYER(player).currentHp >= PLAYER_STAT(player, STAT_MAX_HP))
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    while (count < SECOND_WIND_FOES &&
           (foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && foe->currentHp > 0)
        {
            count++;
        }
    }
    player_addHp(player, (s16)(skill->amount[level - 1] * (1 + count)), FALSE);
    playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
    return SKILL_CAST_SUCCESS;
}

/** A hasted caster's walk carries him further, and Spirit flows in every second. */
static void tickHaste(u8 player, SpecState *st, Actor *actor)
{
    SpecCommand *c = &Commands[player];
    const SpecSkill *const *rows = specRows(player);
    const SpecSkill *skill = c->hasteRow < 13 && rows != NULL ? rows[c->hasteRow] : NULL;
    Vector2Fp16 step = {actor->coords.x - c->lastCoords.x, actor->coords.y - c->lastCoords.y};
    s32 most = HASTE_MAX_STEP << 16;

    c->lastCoords = actor->coords;
    if (--c->hasteFrames == 0 || skill == NULL || c->hasteLevel == 0)
    {
        c->hasteFrames = 0;
        return;
    }
    if (c->hasteFrames % 30 == 0)
    {
        grantSpirit(player, skill->percent);
    }
    if (st->dashFrames != 0 || Hunts[player].leapFrames != 0 || step.x > most || step.x < -most || step.y > most ||
        step.y < -most || (step.x == 0 && step.y == 0))
    {
        return;
    }
    step.x = step.x / 100 * skill->amount[c->hasteLevel - 1];
    step.y = step.y / 100 * skill->amount[c->hasteLevel - 1];
    {
        Vector2Fp16 next = {actor->coords.x + step.x, actor->coords.y + step.y};

        if (hackLib_canLandAt(actor, &next))
        {
            actor->renderCoords.x += step.x;
            actor->renderCoords.y += step.y;
            actor->coords = next;
            c->lastCoords = next;
        }
    }
}

/** The share of the last melee hit lands on every other foe around the caster. */
static void tickCleave(u8 player, Actor *actor)
{
    SpecCommand *c = &Commands[player];
    const SpecSkill *const *rows = specRows(player);
    const SpecSkill *skill = c->cleaveRow < 13 && rows != NULL ? rows[c->cleaveRow] : NULL;
    Actor *victim = hackLib_isActorRefValid(c->cleaveVictim) ? c->cleaveVictim.actor : NULL;
    u16 damage = c->cleaveDamage;
    Actor *foe = NULL;

    c->cleaveDamage = 0;
    if (skill == NULL || damage == 0)
    {
        return;
    }
    while ((foe = hackLib_nextTargetAt(&actor->coords, victim, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        skillExpansion_applyPveRangedHit(actor, foe, damage);
        if (layerUsed(&skill->trail))
        {
            playLayer(&skill->trail, actor, &foe->coords);
        }
    }
}

/** The skill of a running timed effect, or NULL when it is not running. */
static const SpecSkill *timedSkill(u8 player, const SpecTimed *t)
{
    const SpecSkill *const *rows = specRows(player);

    return t->frames != 0 && t->level != 0 && t->row < 13 && rows != NULL ? rows[t->row] : NULL;
}

static void startTimed(SpecTimed *t, const SpecSkill *skill, u8 row, u32 level)
{
    t->frames = skill->frames;
    t->row = row;
    t->level = (u8)level;
}

/** Foes within reach cower for a moment. */
static void dreadPulse(Actor *actor, const SpecSkill *skill)
{
    Actor *foe = NULL;
    u32 count = 0;

    if (layerUsed(&skill->trail))
    {
        playLayer(&skill->trail, actor, NULL);
    }
    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->chargeRadius, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0 || !hackLib_applyStun(foe, skill->stunFrames))
        {
            continue;
        }
        if (count++ < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
    }
}

/** Reflect, dread, undying, absorb, lifesteal, rampart and madness: their work every tick. */
static void tickSelf(u8 player, Actor *actor)
{
    SpecCommand *c = &Commands[player];
    SpecTimed *timers[] = {&c->reflect, &c->dread,   &c->undying, &c->absorb,
                           &c->lifesteal, &c->rampart, &c->madness, &c->plunder, &c->aim};
    const SpecSkill *skill;

    skill = timedSkill(player, &c->aim);
    if (skill != NULL)
    {
        if (actor->coords.x == c->aimCoords.x && actor->coords.y == c->aimCoords.y)
        {
            if (c->stillTicks < 0xffff && ++c->stillTicks % 30 == 0 && layerUsed(&skill->trail))
            {
                playLayer(&skill->trail, actor, NULL);
            }
        }
        else
        {
            c->stillTicks = 0;
        }
        c->aimCoords = actor->coords;
    }
    if (c->pendingHeal != 0)
    {
        player_addHp(player, (s16)c->pendingHeal, FALSE);
        c->pendingHeal = 0;
    }
    if (c->reflectDamage != 0)
    {
        Actor *foe = hackLib_isActorRefValid(c->reflectFoe) ? c->reflectFoe.actor : NULL;

        skill = timedSkill(player, &c->reflect);
        if (foe != NULL && foe->currentHp > 0)
        {
            skillExpansion_applyPveRangedHit(actor, foe, c->reflectDamage);
            if (skill != NULL && layerUsed(&skill->trail))
            {
                playLayer(&skill->trail, actor, &foe->coords);
            }
        }
        c->reflectDamage = 0;
    }
    skill = timedSkill(player, &c->dread);
    if (skill != NULL &&
        c->dread.frames % (skill->trailTicks != 0 ? skill->trailTicks : DREAD_PULSE_TICKS) == 0)
    {
        dreadPulse(actor, skill);
    }
    skill = timedSkill(player, &c->undying);
    if (skill != NULL && c->undying.frames == 1)
    {
        player_addHp(player, (s16)skill->amount[c->undying.level - 1], FALSE);
        if (layerUsed(&skill->trail))
        {
            playLayer(&skill->trail, actor, NULL);
        }
    }
    for (u32 i = 0; i < ARRAY_COUNT(timers); i++)
    {
        if (timers[i]->frames != 0)
        {
            timers[i]->frames--;
        }
    }
    if (c->absorb.frames == 0)
    {
        c->absorbLeft = 0;
    }
}

/** The best planted shield the caster stands behind, in percent less damage, 0 for none. */
static u32 coverPercent(u8 player, Actor *actor)
{
    u32 best = 0;

    for (u32 i = 0; actor != NULL && i < LASTING_COUNT; i++)
    {
        const SpecLasting *l = &Lastings[i];
        s32 dx;
        s32 dy;
        s32 r;

        if (l->kind != LASTING_COVER || l->player != player || l->level == 0)
        {
            continue;
        }
        dx = (actor->coords.x - l->at.x) >> 16;
        dy = (actor->coords.y - l->at.y) >> 16;
        r = l->skill->chargeRadius;
        if (dx * dx + dy * dy <= r * r && l->skill->amount[l->level - 1] > best)
        {
            best = l->skill->amount[l->level - 1];
        }
    }
    return best;
}

/** damage raised by a steady aim: amount percent a second stood still, at most percent. */
static s32 aimed(u8 player, s32 damage)
{
    SpecCommand *c = &Commands[player];
    const SpecSkill *skill = timedSkill(player, &c->aim);
    u32 percent;

    if (skill == NULL)
    {
        return damage;
    }
    percent = c->stillTicks / 30 * skill->amount[c->aim.level - 1];
    percent = percent > skill->percent ? skill->percent : percent;
    return damage + damage * (s32)percent / 100;
}

// Xorshift state, seeded on first use (modcode has no initialized data).
static u32 RandomState;

/** A number 0..99 (xorshift, its own stream). */
static u32 percentRoll(void)
{
    if (RandomState == 0)
    {
        RandomState = 0x2545f491;
    }
    RandomState ^= RandomState << 13;
    RandomState ^= RandomState >> 17;
    RandomState ^= RandomState << 5;
    return RandomState % 100;
}

/** The strongest blind on foe, in percent of its blows that miss. */
static u32 blindPercent(Actor *foe)
{
    u32 percent = 0;

    for (u32 i = 0; i < LASTING_COUNT; i++)
    {
        const SpecLasting *l = &Lastings[i];

        if (l->kind == LASTING_BLIND && hackLib_isActorRefValid(l->foe) && l->foe.actor == foe &&
            l->skill->percent > percent)
        {
            percent = l->skill->percent;
        }
    }
    return percent;
}

/** A blow on the caster after his rampart, madness, absorb, reflect and undying. */
static s32 selfDefense(u8 player, Actor *attacker, s32 damage)
{
    SpecCommand *c = &Commands[player];
    Actor *actor = PLAYER(player).ownerActor;
    const SpecSkill *skill;

    if (attacker != NULL && damage > 0 && percentRoll() < blindPercent(attacker))
    {
        hackLib_fxBurst(attacker, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE, FX_LAYER_FRONT);
        return 0;
    }
    skill = timedSkill(player, &c->rampart);
    if (skill != NULL && attacker != NULL && actor != NULL)
    {
        s32 dx = (attacker->coords.x - actor->coords.x) >> 16;
        s32 dy = (attacker->coords.y - actor->coords.y) >> 16;

        if (dx * dx + dy * dy > skill->chargeRadius * skill->chargeRadius)
        {
            damage -= damage * skill->amount[c->rampart.level - 1] / 100;
        }
    }
    skill = timedSkill(player, &c->madness);
    if (skill != NULL)
    {
        damage += damage * skill->percent / 100;
    }
    damage -= damage * (s32)coverPercent(player, actor) / 100;
    if (c->absorbLeft != 0 && damage > 0)
    {
        s32 soaked = damage < c->absorbLeft ? damage : c->absorbLeft;

        c->absorbLeft -= (u16)soaked;
        damage -= soaked;
        if (actor != NULL)
        {
            hackLib_fxBurst(actor, FX_SHAPE_SPARK, c->absorbLeft != 0 ? FX_COLOR_NAZGUL : FX_COLOR_WHITE, FX_LAYER_FRONT);
        }
    }
    skill = timedSkill(player, &c->reflect);
    if (skill != NULL && attacker != NULL && skillExpansion_isFoe(attacker) && damage > 0)
    {
        s32 back = damage * skill->amount[c->reflect.level - 1] / 100;

        c->reflectFoe = hackLib_actorRef(attacker);
        c->reflectDamage = (u16)(back < 1 ? 1 : back > 0xffff ? 0xffff : back);
    }
    if (timedSkill(player, &c->undying) != NULL && damage >= PLAYER(player).currentHp)
    {
        damage = PLAYER(player).currentHp - 1;
        if (actor != NULL)
        {
            hackLib_fxBurst(actor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_NIGHT, FX_LAYER_FRONT);
        }
    }
    return damage < 0 ? 0 : damage;
}

/** A bolt's look flying range pixels along dir (a unit step) from the caster. */
static void launchFlight(Actor *actor, u8 player, const SpecSkill *skill, u32 level, Vector2Fp16 dir, s32 range)
{
    SpecLasting *l;

    if (!layerUsed(&skill->trail) ||
        (l = addLasting(LASTING_FLIGHT, skill, player, level, &actor->coords, NULL, FALSE)) == NULL)
    {
        return;
    }
    l->step.x = dir.x * BOLT_SPEED;
    l->step.y = dir.y * BOLT_SPEED;
    l->left = (u16)(range / BOLT_SPEED + 1);
}

/** Every foe within chargeRadius of a bolt's line along dir, targetRange long, is struck. */
static void boltLine(Actor *actor, const SpecSkill *skill, u32 level, Vector2Fp16 dir)
{
    Actor *foe = NULL;
    u32 hit = 0;

    while ((foe = hackLib_nextTargetAt(&actor->coords, NULL, skill->targetRange, foe)) != NULL)
    {
        s32 ahead;
        s32 aside;

        if (!skillExpansion_isFoe(foe) || foe->currentHp <= 0)
        {
            continue;
        }
        offsetAlong(actor, foe, dir, &ahead, &aside);
        if (ahead < 0 || ahead > skill->targetRange || aside > skill->chargeRadius)
        {
            continue;
        }
        touch(actor, foe, skill, level);
        if (foe->currentHp > 0)
        {
            hackLib_applyStun(foe, skill->stunFrames);
        }
        if (hit++ < LOOK_FOES)
        {
            playFx(skill, actor, foe, NULL, SPEC_PLAY_TARGET);
        }
    }
}

/** A burning bolt at the nearest foe; the foe burns, and if it dies in time the fire spreads. */
static SkillExpansionCastResult ignite(Actor *actor, u8 player, const SpecSkill *skill, u32 level)
{
    Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);
    s32 dx;
    s32 dy;
    s32 length;
    Vector2Fp16 dir;

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    dx = (foe->coords.x - actor->coords.x) >> 16;
    dy = (foe->coords.y - actor->coords.y) >> 16;
    length = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    length = length < 1 ? 1 : length;
    dir.x = (dx << 16) / length;
    dir.y = (dy << 16) / length;
    actor->facing = (Direction8)actor_direction8Toward(actor, foe);
    launchFlight(actor, player, skill, level, dir, length);
    touch(actor, foe, skill, level);
    playFx(skill, actor, foe, NULL, SPEC_PLAY_ALL);
    if (foe->currentHp > 0)
    {
        addLasting(LASTING_IGNITE, skill, player, level, &foe->coords, foe, FALSE);
    }
    else
    {
        SpecLasting fell = {.skill = skill, .at = foe->coords, .player = player, .level = (u8)level};

        igniteSpread(&fell, actor);
    }
    return SKILL_CAST_SUCCESS;
}

// Per player: the melee frame seen last tick plus 1, 0 when not swinging.
static u8 PrimaryFrames[2];

/** A missile primary: each swing looses its bolt as it reaches the spec's frame. */
static void tickPrimary(u8 player, Actor *actor)
{
    const SpecKitTable *table = specTable(player);
    const SpecPrimary *primary = table != NULL ? table->primary : NULL;
    u8 frame = actor->currentAnimationFrame;
    bool32 swinging;

    if (primary == NULL)
    {
        return;
    }
    swinging = actor->as.combat.head.player.animationBank == HERO_MELEE_BANK(PLAYER(player).heroId);
    if (swinging && frame == primary->frame && PrimaryFrames[player] != frame + 1)
    {
        u32 level = 1 + PLAYER(player).level / 5;
        u8 direction = actor->facing < 8 ? actor->facing * 4 : 0;

        skillDelivery_launch(player, primary->delivery, &actor->coords, direction, (u8)(level > 5 ? 5 : level),
                             HACKLIB_ABILITY_NONE, FALSE);
    }
    PrimaryFrames[player] = swinging ? (u8)(frame + 1) : 0;
}

SkillExpansionCastResult specKit_resolve(Actor *actor)
{
    u8 player = actor->as.combat.playerIndex;
    u8 row = PLAYER(player).castingActiveSkill;
    const SpecSkill *const *rows = specRows(player);
    u32 level = skillExpansion_effectiveActiveLevel(player, row);
    const SpecSkill *skill;
    SpecState *st;
    SkillExpansionCastResult result = SKILL_CAST_SUCCESS;
    Actor *target = NULL;

    if (rows == NULL || row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT || rows[row] == NULL || level == 0)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    skill = rows[row];
    level = level > 5 ? 5 : level;
    st = state(player);
    switch (skill->effect)
    {
    case SPEC_EFFECT_DELIVERY:
        if (!launch(actor, player, skill, (u8)level, row, &target))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        playFx(skill, actor, target, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_HEAL:
        if (PLAYER(player).currentHp >= PLAYER_STAT(player, STAT_MAX_HP))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        player_addHp(player, (s16)skill->amount[level - 1], FALSE);
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_WARD:
        st->wardPercent = (u8)skill->amount[level - 1];
        st->wardFrames = skill->frames;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_EMPOWER:
        st->empowerPercent = (u8)skill->amount[level - 1];
        st->empowerFrames = skill->frames;
        st->empowerRow = row;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_SUMMON:
        result = summon(actor, skill, level);
        break;
    case SPEC_EFFECT_DEVOUR:
        result = devour(actor, player, skill, level);
        break;
    case SPEC_EFFECT_TITHE:
        result = tithe(actor, player, skill, level);
        break;
    case SPEC_EFFECT_CHARGE:
        result = charge(actor, player, skill, level);
        break;
    case SPEC_EFFECT_SWOOP:
        result = swoop(actor, player, skill, level, row);
        break;
    case SPEC_EFFECT_FEAR:
        result = afflict(actor, player, skill, level, LASTING_FEAR);
        break;
    case SPEC_EFFECT_ROOT:
        result = afflict(actor, player, skill, level, LASTING_ROOT);
        break;
    case SPEC_EFFECT_PHASE:
        st->phaseFrames = skill->frames;
        st->phaseRow = row;
        st->phaseLevel = (u8)level;
        actor->spriteFlags |= SPRITE_FLAG_STEALTH;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_ZONE:
        result = zone(actor, player, skill, level, FALSE);
        break;
    case SPEC_EFFECT_DRAIN:
        result = zone(actor, player, skill, level, TRUE);
        break;
    case SPEC_EFFECT_STAMPEDE:
        result = stampede(actor, player, skill, level);
        break;
    case SPEC_EFFECT_CHARM:
        result = charm(actor, player, skill, level);
        break;
    case SPEC_EFFECT_MARK:
        result = afflict(actor, player, skill, level, LASTING_MARK);
        break;
    case SPEC_EFFECT_DISMAY:
        result = dismay(actor, player, skill, level);
        break;
    case SPEC_EFFECT_DASH:
        st->dashFrames = skill->frames;
        st->dashRow = row;
        st->dashLevel = (u8)level;
        st->dashBack = FALSE;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_BACKSTEP:
        placeTrap(actor, player, skill, level, &actor->coords);
        st->dashFrames = BACKSTEP_FRAMES;
        st->dashRow = row;
        st->dashLevel = (u8)level;
        st->dashBack = TRUE;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_LEAP:
        result = leap(actor, player, skill, level, row);
        break;
    case SPEC_EFFECT_WEAKEN:
        result = afflict(actor, player, skill, level, LASTING_WEAKEN);
        break;
    case SPEC_EFFECT_BLEED:
    case SPEC_EFFECT_VENOM:
        Hunts[player].onHitFrames = skill->frames;
        Hunts[player].onHitRow = row;
        Hunts[player].onHitLevel = (u8)level;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_FINISHER:
        result = finisher(actor, skill, level);
        break;
    case SPEC_EFFECT_FRENZY:
    {
        const SpecFrenzy *frenzy = specFrenzy(player);
        u32 stacks;

        if (frenzy == NULL)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        Hunts[player].cap = skill->count;
        Hunts[player].capFrames = skill->frames;
        stacks = Hunts[player].stacks + skill->amount[level - 1];
        Hunts[player].stacks = (u8)(stacks > frenzyCap(player, frenzy) ? frenzyCap(player, frenzy) : stacks);
        Hunts[player].idleFrames = 0;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    }
    case SPEC_EFFECT_TRAP:
        result = traps(actor, player, skill, level);
        break;
    case SPEC_EFFECT_PULL:
        result = seize(actor, player, skill, level, LASTING_PULL);
        break;
    case SPEC_EFFECT_SLEEP:
        result = seize(actor, player, skill, level, LASTING_SLEEP);
        break;
    case SPEC_EFFECT_BOUNTY:
        st->bountyFrames = skill->frames;
        st->bountyRow = row;
        st->bountyLevel = (u8)level;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_BARRAGE:
        result = barrage(actor, player, skill, level);
        break;
    case SPEC_EFFECT_DESPAIR:
    {
        s32 cost = PLAYER_STAT(player, STAT_MAX_HP) * skill->percent / 100;

        if (PLAYER(player).currentHp <= cost)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        skillExpansion_original_player_addHp(player, (s16)-cost, FALSE);
        st->despairFrames = skill->frames;
        st->despairPercent = skill->amount[level - 1];
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    }
    case SPEC_EFFECT_LASH:
        result = lash(actor, player, skill, level);
        break;
    case SPEC_EFFECT_SHOVE:
        result = shove(actor, player, skill, level);
        break;
    case SPEC_EFFECT_TAUNT:
        result = taunt(actor, st, skill, level);
        break;
    case SPEC_EFFECT_HASTE:
        if (player >= 2)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        Commands[player].hasteFrames = skill->frames;
        Commands[player].hasteRow = row;
        Commands[player].hasteLevel = (u8)level;
        Commands[player].lastCoords = actor->coords;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_SECOND_WIND:
        result = secondWind(actor, player, skill, level);
        break;
    case SPEC_EFFECT_CLEAVE:
        if (player >= 2)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        Commands[player].cleaveFrames = skill->frames;
        Commands[player].cleaveRow = row;
        Commands[player].cleaveLevel = (u8)level;
        Commands[player].cleaveDamage = 0;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_ALL);
        break;
    case SPEC_EFFECT_REFLECT:
    case SPEC_EFFECT_DREAD:
    case SPEC_EFFECT_UNDYING:
    case SPEC_EFFECT_ABSORB:
    case SPEC_EFFECT_LIFESTEAL:
    case SPEC_EFFECT_RAMPART:
    case SPEC_EFFECT_MADNESS:
    {
        SpecCommand *c = &Commands[player < 2 ? player : 0];
        SpecTimed *t = skill->effect == SPEC_EFFECT_REFLECT     ? &c->reflect
                       : skill->effect == SPEC_EFFECT_DREAD     ? &c->dread
                       : skill->effect == SPEC_EFFECT_UNDYING   ? &c->undying
                       : skill->effect == SPEC_EFFECT_ABSORB    ? &c->absorb
                       : skill->effect == SPEC_EFFECT_LIFESTEAL ? &c->lifesteal
                       : skill->effect == SPEC_EFFECT_RAMPART   ? &c->rampart
                                                                : &c->madness;

        if (player >= 2)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        startTimed(t, skill, row, level);
        if (skill->effect == SPEC_EFFECT_ABSORB)
        {
            c->absorbLeft = skill->amount[level - 1];
        }
        if (skill->effect == SPEC_EFFECT_DREAD)
        {
            dreadPulse(actor, skill);
        }
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    }
    case SPEC_EFFECT_GRAPPLE:
    {
        Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);

        if (foe == NULL || player >= 2)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        for (s32 d = 12; layerUsed(&skill->trail) && d < 255; d += 12)
        {
            s32 dx = (foe->coords.x - actor->coords.x) >> 16;
            s32 dy = (foe->coords.y - actor->coords.y) >> 16;
            s32 length = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            Vector2Fp16 at = actor->coords;

            if (d >= length)
            {
                break;
            }
            at.x += (foe->coords.x - actor->coords.x) / length * d;
            at.y += (foe->coords.y - actor->coords.y) / length * d;
            playLayer(&skill->trail, actor, &at);
        }
        result = leap(actor, player, skill, level, row);
        break;
    }
    case SPEC_EFFECT_FLURRY:
    {
        Actor *foe = skillExpansion_findTarget(actor, skill->targetRange, NULL, 0);
        SpecLasting *l;

        if (foe == NULL || (l = addLasting(LASTING_FLURRY, skill, player, level, &foe->coords, foe, FALSE)) == NULL)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        l->stacks = skill->count != 0 ? skill->count : 5;
        l->left = (u16)(l->stacks * (skill->trailTicks != 0 ? skill->trailTicks : 6) + 2);
        actor->facing = (Direction8)actor_direction8Toward(actor, foe);
        playFx(skill, actor, foe, NULL, SPEC_PLAY_ALL);
        break;
    }
    case SPEC_EFFECT_BLIND:
        result = afflict(actor, player, skill, level, LASTING_BLIND);
        break;
    case SPEC_EFFECT_PLUNDER:
        if (player >= 2)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        startTimed(&Commands[player].plunder, skill, row, level);
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    case SPEC_EFFECT_BOMB:
    {
        Vector2Fp16 unit = facingUnit(actor);
        Vector2Fp16 at = {actor->coords.x + unit.x * BOMB_AHEAD, actor->coords.y + unit.y * BOMB_AHEAD};
        SpecLasting *l;

        if (!hackLib_canLandAt(actor, &at))
        {
            at = actor->coords;
        }
        l = addLasting(LASTING_BOMB, skill, player, level, &at, NULL, FALSE);
        if (l == NULL)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        if (layerUsed(&skill->trail))
        {
            l->look = spawnLook(&skill->trail, actor, &at, skill->frames);
        }
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    }
    case SPEC_EFFECT_BROADSIDE:
    {
        SpecLasting *l = addLasting(LASTING_SALVO, skill, player, level, &actor->coords, NULL, FALSE);
        u32 count = skill->count != 0 ? skill->count : 5;

        if (l == NULL)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        l->stacks = actor->facing < 8 ? actor->facing : 2;
        l->left = (u16)(count * (skill->trailTicks != 0 ? skill->trailTicks : 6) + 1);
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    }
    case SPEC_EFFECT_PIERCE:
    {
        Vector2Fp16 unit = facingUnit(actor);

        boltLine(actor, skill, level, unit);
        launchFlight(actor, player, skill, level, unit, skill->targetRange);
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    }
    case SPEC_EFFECT_FAN:
    {
        Vector2Fp16 unit = facingUnit(actor);
        u32 count = skill->count != 0 ? skill->count : 3;

        for (u32 k = 0; k < count; k++)
        {
            s32 side = (s32)(2 * k) - (s32)(count - 1);
            Vector2Fp16 dir = {unit.x - unit.y * side * FAN_SPREAD / 200, unit.y + unit.x * side * FAN_SPREAD / 200};

            boltLine(actor, skill, level, dir);
            launchFlight(actor, player, skill, level, dir, skill->targetRange);
        }
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    }
    case SPEC_EFFECT_COVER:
    {
        Vector2Fp16 unit = facingUnit(actor);
        Vector2Fp16 at = {actor->coords.x + unit.x * BOMB_AHEAD / 2, actor->coords.y + unit.y * BOMB_AHEAD / 2};
        SpecLasting *l;

        if (!hackLib_canLandAt(actor, &at))
        {
            at = actor->coords;
        }
        l = addLasting(LASTING_COVER, skill, player, level, &at, NULL, FALSE);
        if (l == NULL)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        if (layerUsed(&skill->trail))
        {
            l->look = spawnLook(&skill->trail, actor, &at, skill->frames);
        }
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    }
    case SPEC_EFFECT_IGNITE:
        result = ignite(actor, player, skill, level);
        break;
    case SPEC_EFFECT_AIM:
        if (player >= 2)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        startTimed(&Commands[player].aim, skill, row, level);
        Commands[player].stillTicks = 0;
        Commands[player].aimCoords = actor->coords;
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    case SPEC_EFFECT_HAIL:
        if (addLasting(LASTING_HAIL, skill, player, level, &actor->coords, NULL, FALSE) == NULL)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        playFx(skill, actor, NULL, NULL, SPEC_PLAY_SELF);
        break;
    default:
        return SKILL_CAST_UNAVAILABLE;
    }
    if (result == SKILL_CAST_SUCCESS && skill->burst != NULL)
    {
        u8 direction = actor->facing < 8 ? actor->facing * 4 : 0;

        skillDelivery_launch(player, skill->burst, &actor->coords, direction, (u8)level, row, FALSE);
    }
    if (result == SKILL_CAST_SUCCESS && skill->rallyFrames != 0)
    {
        rally(actor, skill->rallyFrames);
    }
    if (result == SKILL_CAST_SUCCESS && skill->sound != 0)
    {
        sfx_play(skill->sound);
    }
    return result;
}

void specKit_reset(u8 playerIndex)
{
    if (specRows(playerIndex) != NULL)
    {
        SpecState *st = state(playerIndex);
        const SpecRebirth *rebirth = specRebirth(playerIndex);

        for (u32 i = 0; i < CUE_COUNT; i++)
        {
            if (Cues[i].player == playerIndex)
            {
                Cues[i].fx = NULL;
            }
        }
        for (u32 i = 0; i < LASTING_COUNT; i++)
        {
            if (Lastings[i].player == playerIndex)
            {
                Lastings[i].kind = LASTING_NONE;
            }
        }
        st->phaseFrames = 0;
        st->dashFrames = 0;
        st->bountyFrames = 0;
        st->despairFrames = 0;
        if (playerIndex < 2)
        {
            Hunts[playerIndex] = (SpecHunt){0};
            Commands[playerIndex] = (SpecCommand){0};
        }

        st->wardFrames = st->empowerFrames = 0;
        st->rebirthFrames = 0;
        st->killsSeen = 0;
        if (rebirth != NULL && st->charges < rebirth->startCharges)
        {
            st->charges = rebirth->startCharges;
        }
    }
}

void specKit_cleanup(u8 playerIndex, Actor *actor)
{
    (void)actor;
    specKit_reset(playerIndex);
}

/** The end of a rebirth: heal, then burst out of the mud. */
static void rise(u8 playerIndex, Actor *actor, const SpecRebirth *rebirth)
{
    SkillExpansionBurst burst = {
        .attacker = actor,
        .center = actor,
        .exclude = NULL,
        .radius = rebirth->burstRadius,
        .damage = rebirth->burstDamage,
        .stunFrames = rebirth->burstStun,
        .kind = HACKLIB_DAMAGE_SECONDARY,
        .abilityRow = HACKLIB_ABILITY_NONE,
    };

    skillExpansion_original_player_addHp(playerIndex,
                                         (s16)(PLAYER_STAT(playerIndex, STAT_MAX_HP) *
                                               (rebirth->healPercent + passiveBonus(playerIndex, SPEC_PASSIVE_REBIRTH_HEAL)) /
                                               100),
                                         FALSE);
    skillExpansion_burstDamage(&burst);
    hackLib_fxBurst(actor, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_NAZGUL, FX_LAYER_BEHIND);
    hackLib_fxBurst(actor, FX_SHAPE_WIDE_RING, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    hackLib_fxBurst(actor, FX_SHAPE_TENDRIL_BURST, FX_COLOR_BLOOD, FX_LAYER_FRONT);
    hackLib_fxBurst(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_NAZGUL, FX_LAYER_FRONT);
    if (rebirth->sound != 0)
    {
        sfx_play(rebirth->sound);
    }
}

void specKit_tick(u8 playerIndex, Actor *actor)
{
    SpecState *st;
    const SpecRebirth *rebirth;

    if (specRows(playerIndex) == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (actor != NULL)
    {
        tickCues(playerIndex, actor);
        tickLastings(playerIndex, actor);
        if (st->phaseFrames != 0)
        {
            tickPhase(playerIndex, st, actor);
        }
        if (st->dashFrames != 0)
        {
            tickDash(playerIndex, st, actor);
        }
        if (Hunts[playerIndex].leapFrames != 0)
        {
            tickLeap(playerIndex, actor);
        }
        if (playerIndex < 2 && Commands[playerIndex].hasteFrames != 0)
        {
            tickHaste(playerIndex, st, actor);
        }
        if (playerIndex < 2 && Commands[playerIndex].cleaveDamage != 0)
        {
            tickCleave(playerIndex, actor);
        }
        if (playerIndex < 2)
        {
            tickSelf(playerIndex, actor);
            tickPrimary(playerIndex, actor);
        }
    }
    if (playerIndex < 2 && Commands[playerIndex].cleaveFrames != 0)
    {
        Commands[playerIndex].cleaveFrames--;
    }
    tickHuntTimers(playerIndex);
    if (st->bountyFrames != 0)
    {
        st->bountyFrames--;
    }
    if (st->despairFrames != 0)
    {
        st->despairFrames--;
    }
    if (st->wardFrames != 0)
    {
        st->wardFrames--;
    }
    if (st->empowerFrames != 0)
    {
        st->empowerFrames--;
    }
    rebirth = specRebirth(playerIndex);
    if (rebirth == NULL || actor == NULL)
    {
        return;
    }
    if (st->flashFrames != 0)
    {
        st->flashFrames--;
    }
    if (!st->killsSeen)
    {
        st->lastKills = PLAYER(playerIndex).killCount;
        st->killsSeen = TRUE;
    }
    else if (PLAYER(playerIndex).killCount != st->lastKills)
    {
        u16 kills = (u16)(PLAYER(playerIndex).killCount - st->lastKills);

        st->lastKills = PLAYER(playerIndex).killCount;
        addProgress(playerIndex, st, rebirth, actor, kills);
    }
    if (st->rebirthFrames != 0)
    {
        st->rebirthFrames--;
        if (st->rebirthFrames % REBIRTH_BUBBLE_FRAMES == 0)
        {
            hackLib_fxBurst(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_GREEN, FX_LAYER_FRONT);
        }
        if (st->rebirthFrames == 0)
        {
            rise(playerIndex, actor, rebirth);
        }
    }
}

/** damage raised by a mark on victim. */
static s32 marked(Actor *victim, s32 damage)
{
    u32 percent = victim != NULL ? markPercent(victim) : 0;

    return damage + damage * (s32)percent / 100;
}

s32 specKit_rangedDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    damage = marked(victim, damage);
    if (specRows(playerIndex) != NULL && state(playerIndex)->despairFrames != 0)
    {
        damage += damage * state(playerIndex)->despairPercent / 100;
    }
    if (specRows(playerIndex) != NULL && playerIndex < 2)
    {
        damage = aimed(playerIndex, damage);
    }
    return damage;
}

void specKit_onKill(u8 playerIndex, Actor *attacker, Actor *victim)
{
    const SpecSkill *const *rows = specRows(playerIndex);
    SpecState *st;
    const SpecSkill *skill;
    Actor *actor = PLAYER(playerIndex).ownerActor;

    (void)attacker;
    if (rows == NULL || actor == NULL)
    {
        return;
    }
    if (playerIndex < 2 && victim != NULL)
    {
        const SpecSkill *plunder = timedSkill(playerIndex, &Commands[playerIndex].plunder);

        if (plunder != NULL && percentRoll() < plunder->amount[Commands[playerIndex].plunder.level - 1])
        {
            Item item;
            u8 lootType = loot_generateDrop(&item, playerIndex,
                                            PLAYER_STAT(playerIndex, STAT_EXTRA_TREASURE_PERCENT), victim->unitType);

            actor_lootPile_create(victim->coords, lootType, item, DROP_DESPAWN_TIMER);
            if (layerUsed(&plunder->trail))
            {
                playLayer(&plunder->trail, actor, &victim->coords);
            }
        }
    }
    st = state(playerIndex);
    skill = st->bountyRow < 13 ? rows[st->bountyRow] : NULL;
    if (st->bountyFrames == 0 || skill == NULL || st->bountyLevel == 0)
    {
        return;
    }
    player_addHp(playerIndex, (s16)skill->amount[st->bountyLevel - 1], FALSE);
    grantSpirit(playerIndex, skill->percent);
    if (layerUsed(&skill->trail))
    {
        playLayer(&skill->trail, actor, NULL);
    }
}

/** A melee hit wakes a sleeping victim. Ranged hits do not: enemy arrows strike other enemies too. */
static void wake(Actor *victim)
{
    for (u32 i = 0; victim != NULL && i < LASTING_COUNT; i++)
    {
        SpecLasting *l = &Lastings[i];

        if (l->kind == LASTING_SLEEP && hackLib_isActorRefValid(l->foe) && l->foe.actor == victim)
        {
            endLasting(l, victim);
        }
    }
}

s32 specKit_meleeDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    const SpecSkill *const *rows = specRows(playerIndex);
    SpecState *st;

    wake(victim);
    damage = marked(victim, damage);
    if (rows == NULL)
    {
        return damage;
    }
    if (playerIndex < 2)
    {
        const SpecFrenzy *frenzy = specFrenzy(playerIndex);
        SpecHunt *h = &Hunts[playerIndex];

        if (h->onHitFrames != 0)
        {
            addStack(playerIndex, PLAYER(playerIndex).ownerActor, victim);
        }
        if (frenzy != NULL)
        {
            if (h->stacks < frenzyCap(playerIndex, frenzy))
            {
                h->stacks++;
            }
            h->idleFrames = 0;
            damage += damage * h->stacks * frenzy->perStack / 100;
        }
        if (Commands[playerIndex].cleaveFrames != 0 && victim != NULL)
        {
            SpecCommand *c = &Commands[playerIndex];
            const SpecSkill *skill = c->cleaveRow < 13 ? rows[c->cleaveRow] : NULL;
            s32 share = skill != NULL && c->cleaveLevel != 0 ? damage * skill->amount[c->cleaveLevel - 1] / 100 : 0;

            c->cleaveVictim = hackLib_actorRef(victim);
            c->cleaveDamage = (u16)(share < 1 ? 1 : share > 0xffff ? 0xffff : share);
        }
        {
            SpecCommand *c = &Commands[playerIndex];
            const SpecSkill *mad = timedSkill(playerIndex, &c->madness);
            const SpecSkill *steal = timedSkill(playerIndex, &c->lifesteal);

            if (mad != NULL)
            {
                damage += damage * mad->amount[c->madness.level - 1] / 100;
            }
            damage = aimed(playerIndex, damage);
            if (steal != NULL && damage > 0)
            {
                s32 heal = c->pendingHeal + damage * steal->amount[c->lifesteal.level - 1] / 100;

                c->pendingHeal = (u16)(heal < 1 ? 1 : heal > 999 ? 999 : heal);
            }
        }
    }
    st = state(playerIndex);
    if (st->empowerFrames == 0)
    {
        return damage;
    }
    if (st->empowerRow < 13 && rows[st->empowerRow] != NULL && rows[st->empowerRow]->status != SKILL_STATUS_NONE &&
        victim != NULL)
    {
        const SpecSkill *skill = rows[st->empowerRow];

        skillStatus_apply(skill->status, PLAYER(playerIndex).ownerActor, victim, skill->statusFrames,
                          skill->statusPower, HACKLIB_ABILITY_NONE);
    }
    return damage + damage * st->empowerPercent / 100;
}

s32 specKit_incomingDamage(u8 playerIndex, Actor *attacker, s32 damage)
{
    SpecState *st;
    const SpecRebirth *rebirth;

    (void)attacker;
    if (damage <= 0 || specRows(playerIndex) == NULL)
    {
        return damage;
    }
    st = state(playerIndex);
    rebirth = specRebirth(playerIndex);
    if ((rebirth != NULL && st->rebirthFrames != 0) || st->phaseFrames != 0 || st->dashFrames != 0 ||
        (playerIndex < 2 && Hunts[playerIndex].leapFrames != 0))
    {
        return 0;
    }
    if (attacker != NULL)
    {
        damage -= damage * (s32)weakenPercent(attacker) / 100;
    }
    if (st->wardFrames != 0)
    {
        damage -= damage * st->wardPercent / 100;
        if (damage < 1)
        {
            damage = 1;
        }
    }
    if (playerIndex < 2)
    {
        damage = selfDefense(playerIndex, attacker, damage);
    }
    if (rebirth != NULL && st->charges != 0 && damage >= PLAYER(playerIndex).currentHp)
    {
        Actor *actor = PLAYER(playerIndex).ownerActor;

        st->charges--;
        st->progress = 0;
        st->rebirthFrames = rebirth->frames;
        st->flashFrames = METER_FLASH_FRAMES;
        if (actor != NULL)
        {
            hackLib_fxTimed(actor, FX_SHAPE_GROUND_POOL, FX_COLOR_NAZGUL, rebirth->frames, FX_LAYER_BEHIND);
            hackLib_fxBurst(actor, FX_SHAPE_SMOKE_RISE, FX_COLOR_GREEN, FX_LAYER_FRONT);
            hackLib_fxBurst(actor, FX_SHAPE_POOL_FLASH, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
        }
        sfx_play(SFX_COMBAT_KNOCKDOWN);
        damage = PLAYER(playerIndex).currentHp - 1;
    }
    return damage;
}

// Mud brown filling, bright on a change, blood red while rising.
static const HackLibHudMeterLook RebirthLook = {HACKLIB_HUD_COLOR_DARK_GOLD, HACKLIB_HUD_COLOR_BROWN,
                                                HACKLIB_HUD_COLOR_NEAR_BLACK, HACKLIB_HUD_COLOR_RED};
static const HackLibHudMeterLook RebirthFlashLook = {HACKLIB_HUD_COLOR_PALE_GOLD, HACKLIB_HUD_COLOR_GOLD,
                                                     HACKLIB_HUD_COLOR_DARK_GOLD, HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook RebirthRisingLook = {HACKLIB_HUD_COLOR_EMBER, HACKLIB_HUD_COLOR_RED,
                                                      HACKLIB_HUD_COLOR_DARK_RED, HACKLIB_HUD_COLOR_WHITE};

void specKit_drawHud(u8 playerIndex)
{
    const SpecRebirth *rebirth = specRebirth(playerIndex);
    SpecState *st;
    u8 marks[4];
    u32 markCount = 0;
    s32 per;
    u32 most;
    const HackLibHudMeterLook *look;
    const SpecFrenzy *frenzy = specFrenzy(playerIndex);

    if (frenzy != NULL && playerIndex < 2)
    {
        hackLib_drawHudMeter(&HackLibHudGapMeter, Hunts[playerIndex].stacks, (s32)frenzyCap(playerIndex, frenzy),
                             Hunts[playerIndex].cap != 0 ? &RebirthFlashLook : &RebirthRisingLook, NULL, 0);
        return;
    }
    if (rebirth == NULL || rebirth->maxCharges == 0)
    {
        return;
    }
    st = state(playerIndex);
    per = (s32)killsPerCharge(playerIndex, rebirth);
    most = maxCharges(playerIndex, rebirth);
    for (u32 i = 1; i < most && markCount < ARRAY_COUNT(marks); i++)
    {
        marks[markCount++] = (u8)(i * per);
    }
    look = st->rebirthFrames != 0 ? &RebirthRisingLook : st->flashFrames != 0 ? &RebirthFlashLook : &RebirthLook;
    hackLib_drawHudMeter(&HackLibHudGapMeter, st->charges * per + st->progress, (s32)most * per, look, marks,
                         markCount);
}

#endif
