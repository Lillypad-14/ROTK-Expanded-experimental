#if HACK_skill_expansion

#include "patches/ringwraith/ringwraith_kit.h"
#include "enemy.h"
#include "fx.h"
#include "lib/actor_ref.h"
#include "lib/fx_kit.h"
#include "lib/fx_sprites.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/timer.h"
#include "memory.h"
#include "patches/ringwraith/ringwraith_icons.h"
#include "patches/ringwraith/wraith_gear.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

// Dread
#define DREAD_MAX 100
#define DREAD_FRIGHTENED 50
#define DREAD_PER_HIT 20
#define DREAD_PER_HIT_PER_TERROR_LEVEL 5
#define DREAD_MARK_COUNT 24
#define PULSE_FRAMES 30
#define AURA_RADIUS 40
#define AURA_RADIUS_PER_LEVEL 10
#define AURA_DREAD 12
#define DREAD_DECAY 6
#define TERROR_STUN_FRAMES 75
#define TERROR_STUN_FRAMES_PER_LEVEL 15
#define TERROR_COOLDOWN_FRAMES 300
#define FRIGHTENED_PERCENT 20
#define FRIGHTENED_PERCENT_PER_LEVEL 4

// Spirit
#define START_SPIRIT_PERCENT 25
#define KILL_SPIRIT 6
#define KILL_SPIRIT_PER_LEVEL 3
#define TERROR_SPIRIT 3
#define TERROR_SPIRIT_PER_LEVEL 1
#define FRIGHTENED_HIT_SPIRIT 1

// Morgul Blade
#define MORGUL_RANGE 48
#define MORGUL_BASE_DAMAGE 8
#define MORGUL_DAMAGE_PER_LEVEL 4
#define SHARD_FRAMES 360
#define SHARD_BASE_DAMAGE 2
#define THRALL_FRAMES 600
#define THRALL_FRAMES_PER_LEVEL 180
#define THRALL_BASE_DAMAGE 8
#define THRALL_DAMAGE_PER_LEVEL 4
#define THRALL_BASE_MAX 2

// Cry of the Nazgul
#define CRY_BASE_DAMAGE 2
#define CRY_DAMAGE_PER_LEVEL 2

// The Unseen World
#define UNSEEN_BASE_FRAMES 150
#define UNSEEN_FRAMES_PER_LEVEL 30
#define UNSEEN_DAMAGE_MULTIPLIER 3
#define UNSEEN_SFX 0x47

// Fell Beast
#define FELL_BEAST_BAND 40
#define FELL_BEAST_BASE_DAMAGE 14
#define FELL_BEAST_DAMAGE_PER_LEVEL 7
#define FELL_BEAST_DREAD 50
#define FELL_BEAST_STUN_FRAMES 40

// No Living Man
#define MALICE_BASE_FRAMES 180
#define MALICE_FRAMES_PER_LEVEL 45
#define MALICE_FRAMES_PER_MALICE_LEVEL 60
#define MALICE_HEAL_PERCENT 8
#define MALICE_HEAL_PERCENT_PER_LEVEL 3
#define MALICE_KILL_SPIRIT 4

// Black Breath
#define BREATH_FRAMES 360
#define BREATH_RADIUS 56
#define BREATH_BASE_DAMAGE 3
#define BREATH_DREAD_PER_DAMAGE 20
#define DRAIN_HEAL_PERCENT 60
#define DRAIN_HEAL_PERCENT_PER_LEVEL 20

// Passives
#define WRAITH_FORM_PERCENT_PER_LEVEL 6
#define WRAITH_FORM_WEAKNESS_PERCENT 20
#define MACE_PERCENT_PER_LEVEL 6

// Command of the Nine
#define COMMAND_RADIUS 48
#define COMMAND_BASE_DAMAGE 12
#define COMMAND_DAMAGE_PER_LEVEL 6
#define COMMAND_DREAD 60
#define COMMAND_SPIRIT_PER_THRALL 8

// Shadowstep
#define SHADOWSTEP_RANGE 96
#define SHADOWSTEP_OFFSET 16
#define SHADOWSTEP_BASE_FRAMES 45
#define SHADOWSTEP_FRAMES_PER_LEVEL 15

// Shadow of Mordor
#define MORDOR_RADIUS 56
#define MORDOR_BASE_FRAMES 240
#define MORDOR_FRAMES_PER_LEVEL 60
#define MORDOR_DREAD 15
#define MORDOR_BASE_DAMAGE 2
#define MORDOR_DAMAGE_PER_LEVEL 2

// Black Sacrifice
#define SACRIFICE_HP_PERCENT 25
#define SACRIFICE_BASE_SPIRIT 30
#define SACRIFICE_SPIRIT_PER_LEVEL 10
#define SACRIFICE_FRAMES 300
#define SACRIFICE_DRAIN_MULTIPLIER 2

// Will of Sauron
#define WILL_BASE_FRAMES 600
#define WILL_FRAMES_PER_LEVEL 60

// Page 2 passives
#define HOST_THRALLS_PER_LEVEL 1
#define GRAVE_PACT_FRAMES_PER_LEVEL 60
#define GRAVE_PACT_BASE_DAMAGE 6
#define GRAVE_PACT_DAMAGE_PER_LEVEL 3
#define GRAVE_PACT_DREAD 40
#define MASTER_DAMAGE_PENALTY_PERCENT 30
#define FADING_FRAMES_PER_LEVEL 30
#define FADING_REFUND_PERCENT_PER_LEVEL 10
#define RINGBOUND_REFUND_PERCENT 50
#define KNIFE_BASE_FRAMES 45
#define KNIFE_FRAMES_PER_LEVEL 15
#define DEATH_FROM_SHADOW_EXTRA_MULTIPLIER 2
#define DEATH_FROM_SHADOW_PENALTY_PERCENT 40
#define DREAD_LORD_PERCENT_PER_LEVEL 4
#define FEAST_HEAL_PERCENT_PER_LEVEL 2
#define FEAST_SPIRIT_PER_LEVEL 2

// Thralls of one Command of the Nine that burn in Nazgul fire, the rest flash.
#define COMMAND_FIRE_THRALLS 3
// Ticks per frame of the animations played as effects.
#define NAZGUL_FIRE_TICKS 4
#define MORDOR_SHADE_TICKS 5
// Ticks per frame of the stock shapes a cast plays on the wraith.
#define CAST_FX_TICKS 3
// Pixels Black Breath's smoke starts above the feet.
#define BREATH_SMOKE_LIFT 14

// Morgul Blade blade glow: the blade's palette colors flare toward BLADE_GLOW_COLOR and fade.
#define BLADE_FIRST_COLOR 8
#define BLADE_COLOR_COUNT 5
#define BLADE_GLOW_COLOR 0x73fd
#define BLADE_GLOW_FRAMES 72
#define BLADE_GLOW_RISE_FRAMES 6
#define BLADE_SPARK_INTERVAL 8

// Actor.spriteFlags bit of a stealthed (see-through) player.
#define SPRITE_FLAG_STEALTH 0x20

// Wraith gear uniques.
#define WITCH_KINGS_MACE_RADIUS 48
#define WITCH_KINGS_MACE_DREAD 40
#define MORGUL_KNIFE_PENALTY_PERCENT 25
#define IRON_CROWN_DAMAGE_TAKEN_PERCENT 15
#define NINTH_RING_HITS 9
#define SHROUD_UNSEEN_MULTIPLIER 5
#define BLACK_CAPTAIN_DRAIN_PERCENT 10
#define MORGUL_VALE_FRAMES 120
#define MORGUL_VALE_RANGE 96
#define MORGUL_VALE_DREAD 25
#define BANNER_EXTRA_THRALLS 1
#define BANNER_THRALL_DAMAGE 3
#define BARROW_CROWN_RADIUS 32
#define BARROW_CROWN_DREAD 15
#define DEAD_MARSHES_UNSEEN_PERCENT 50
#define HORN_DAMAGE_MULTIPLIER 2
#define HORN_REFUND_PERCENT 50
#define MASK_RADIUS 48
#define MASK_DAMAGE 8
#define MASK_DREAD 50
#define MASK_MAX_CHAIN 3
#define KHAMUL_HEAL_PERCENT 3
#define SADDLE_BAND_MULTIPLIER 2

// Page 1 rows are the kit's stock rows. Page 2 rows follow the SkillExpansionActiveDefs order.
enum RingwraithRow
{
    ROW_MORGUL_BLADE = 0,
    ROW_CRY_OF_THE_NAZGUL = 1,
    ROW_UNSEEN_WORLD = 2,
    ROW_FELL_BEAST = 3,
    ROW_NO_LIVING_MAN = 4,
    ROW_BLACK_BREATH = ACTIVE_SKILL_HERBAL_HEALING,
    ROW_COMMAND_OF_THE_NINE = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW,
    ROW_SHADOWSTEP = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 1,
    ROW_SHADOW_OF_MORDOR = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 2,
    ROW_BLACK_SACRIFICE = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 3,
    ROW_WILL_OF_SAURON = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 4,
};

typedef struct DreadMark
{
    // marked foe, NULL for a free mark
    Actor *actor;
    u32 generation;

    // frames left of the Morgul shard
    u16 shardFrames;

    // frames before the foe can be terrified again
    u16 terrorFrames;

    // 0..DREAD_MAX
    u8 dread;

    // unitType at marking, to drop marks on reused actors
    u8 unitType;

    // playerIndex of the shard's wraith
    u8 shardOwner;

    // Morgul Blade level of the shard
    u8 shardLevel;
} DreadMark;

typedef struct RingwraithState
{
    // Black Breath frames left
    u16 breathFrames;

    // The Unseen World frames left
    u16 unseenFrames;

    // No Living Man frames left
    u16 maliceFrames;

    // frames until the next Dread and drain pulse
    u16 pulseFrames;

    // currentSpirit after the last tick or grant, -1 before the first tick
    s16 lastSpirit;

    // TRUE while a heal or Spirit gain of the kit applies
    u8 allowGain;

    // TRUE while a melee blow from The Unseen World resolves
    u8 unseenStrike;

    // melee hits landed, for The Ninth Ring
    u8 meleeHits;

    // frames until the Greaves of the Morgul Vale strike
    u8 valeFrames;

    // percent of the pending melee blow's damage that heals the wraith
    u8 drainPercent;

    // fractional healing, in hundredths of a hit point
    u16 drainRemainder;

    // Black Sacrifice frames left
    u16 sacrificeFrames;

    // Will of Sauron frames left
    u16 willFrames;

    // Shadow of Mordor frames left
    u16 mordorFrames;

    // Shadow of Mordor ground center
    Vector2Fp16 mordorCenter;

    // Shadow of Mordor cast level
    u8 mordorLevel;

    // TRUE while the Unseen World came from Shadowstep
    u8 stepStrike;

    // Mask of Dol Guldur bursts nested in the current kill
    u8 maskChain;

    // Morgul Blade blade glow frames left
    u8 bladeGlowFrames;
} RingwraithState;

static DreadMark *marks;
static RingwraithState states[2];

Actor *ringwraith_kitEnemyAttacker;

// Each original runs the 8-byte prologue that its hookReplace overwrote, then continues past it.
asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global ringwraith_original_player_addSpirit\n"
    ".thumb_func\n"
    "ringwraith_original_player_addSpirit:\n"
    "    push {r4, r5, lr}\n"
    "    lsl  r0, r0, #24\n"
    "    lsr  r0, r0, #24\n"
    "    add  r4, r0, #0\n"
    "    ldr  r3, =player_addSpirit + 8\n"
    "    bx   r3\n"
    "    .pool\n");

void ringwraith_original_player_addSpirit(u32 playerIndex, s32 delta);

static Actor *nearestFoe(Actor *playerActor, s32 range, u32 minDread);

static inline s32 gear(u8 playerIndex, u32 effect)
{
    return wraithGear_effect(playerIndex, effect);
}

static inline bool32 hasUnique(u8 playerIndex, u32 unique)
{
    return wraithGear_hasUnique(playerIndex, unique);
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline u8 castLevel(u8 playerIndex)
{
    return *skillExpansion_activeLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

static inline bool32 isWraithAvatar(Actor *actor)
{
    return skillExpansion_isPlayerAvatar(actor) && ringwraith_isHero(actor->as.combat.playerIndex);
}

static s32 percentOf(s32 value, s32 percent)
{
    return value * percent / 100;
}

static void heal(u8 playerIndex, s32 amount)
{
    s32 missing = PLAYER_STAT(playerIndex, STAT_MAX_HP) - PLAYER(playerIndex).currentHp;

    if (amount > missing)
    {
        amount = missing;
    }
    if (amount < 1)
    {
        return;
    }
    states[playerIndex].allowGain = TRUE;
    player_addHp(playerIndex, amount, FALSE);
    states[playerIndex].allowGain = FALSE;
}

/** Heal from damage actually dealt, retaining fractional HP between hits. */
static void healDrain(u8 playerIndex, s32 damage, s32 percent)
{
    RingwraithState *st = &states[playerIndex];
    s32 scaled;

    if (damage <= 0 || percent <= 0)
    {
        return;
    }
    scaled = damage * percent;
    scaled +=
        percentOf(scaled, gear(playerIndex, WRAITH_FX_DRAIN_HEAL) +
                              DRAIN_HEAL_PERCENT_PER_LEVEL *
                                  passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SHADOW_HUNGER));
    if (st->sacrificeFrames != 0)
    {
        scaled *= SACRIFICE_DRAIN_MULTIPLIER;
    }
    scaled += st->drainRemainder;
    st->drainRemainder = scaled % 100;
    heal(playerIndex, scaled / 100);
}

static void grantSpirit(u8 playerIndex, s32 amount)
{
    RingwraithState *st = &states[playerIndex];

    st->allowGain = TRUE;
    player_addSpirit(playerIndex, amount);
    st->allowGain = FALSE;
    st->lastSpirit = PLAYER(playerIndex).currentSpirit;
}

/**
 * A stock shape on the wraith at a cast, slowed so it reads as the skill's signature. SMOKE_RISE
 * starts below the feet, so lift it with castFxRaised.
 */
static Actor *castFx(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(playerActor, shape, color, layer), CAST_FX_TICKS);
}

/** castFx left at the wraith's position, raised by lift pixels. */
static void castFxRaised(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer, s32 lift)
{
    Actor *fx = castFx(playerActor, shape, color, layer);
    Vector2Fp16 pos = playerActor->coords;

    if (fx != NULL)
    {
        pos.y -= lift << 16;
        hackLib_fxDetach(fx, &pos);
    }
}

static void frightFx(Actor *foe)
{
    hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_NIGHT, FX_LAYER_FRONT);
}

static void terrorFx(Actor *foe)
{
    hackLib_fxBurst(foe, FX_SHAPE_TENDRIL_STAR, FX_COLOR_NIGHT, FX_LAYER_FRONT);
}

static void shardFx(Actor *foe)
{
    hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_MORGUL, FX_LAYER_FRONT);
}

static void gloomFx(Actor *foe)
{
    hackLib_fxBurst(foe, FX_SHAPE_SMOKE_SMALL, FX_COLOR_NIGHT, FX_LAYER_FRONT);
}

/** Effect left at pos, standing in for actor when no shared palette slot is free. */
static Actor *fxAtOr(Actor *origin, const Vector2Fp16 *pos, u8 shape, HackLibFxColor color,
                     HackLibFxColor fallback)
{
    Actor *fx = hackLib_fxAt(origin, pos, shape, color);

    return fx != NULL ? fx : hackLib_fxAt(origin, pos, shape, fallback);
}

static void stun(Actor *foe, s32 frames)
{
    if (skillExpansion_isFoe(foe))
    {
        hackLib_applyStun(foe, frames);
    }
}

static DreadMark *findMark(Actor *foe)
{
    if (marks == NULL)
    {
        if (foe == NULL || (marks = memory_zalloc(sizeof(*marks) * DREAD_MARK_COUNT)) == NULL)
        {
            return NULL;
        }
    }
    u32 i;

    for (i = 0; i < DREAD_MARK_COUNT; i++)
    {
        if (marks[i].actor != NULL &&
            (marks[i].generation == 0 ||
             hackLib_actorGeneration(marks[i].actor) != marks[i].generation))
        {
            marks[i].actor = NULL;
        }
        if (marks[i].actor == foe)
        {
            return &marks[i];
        }
    }
    return NULL;
}

static DreadMark *getMark(Actor *foe)
{
    DreadMark *mark = findMark(foe);

    if (mark == NULL && skillExpansion_isFoe(foe) && (mark = findMark(NULL)) != NULL)
    {
        memory_memset(mark, 0, sizeof(*mark));
        mark->actor = foe;
        mark->generation = hackLib_actorGeneration(foe);
        mark->unitType = foe->unitType;
    }
    return mark;
}

static u32 dreadOf(Actor *foe)
{
    DreadMark *mark = findMark(foe);

    return mark != NULL ? mark->dread : 0;
}

/** Raise a foe's Dread. Reaching DREAD_MAX freezes it in terror. */
static void addDread(u8 playerIndex, Actor *foe, s32 amount)
{
    DreadMark *mark;
    s32 before;
    s32 dread;

    if (!skillExpansion_isFoe(foe) || (mark = getMark(foe)) == NULL)
    {
        return;
    }
    before = mark->dread;
    dread = before + amount;
    mark->dread = dread > DREAD_MAX ? DREAD_MAX : dread < 0 ? 0 : dread;
    if (mark->dread == DREAD_MAX && mark->terrorFrames == 0)
    {
        mark->terrorFrames = TERROR_COOLDOWN_FRAMES;
        stun(foe, TERROR_STUN_FRAMES + gear(playerIndex, WRAITH_FX_TERROR_STUN) +
                      TERROR_STUN_FRAMES_PER_LEVEL *
                          passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR));
        grantSpirit(playerIndex,
                    TERROR_SPIRIT + gear(playerIndex, WRAITH_FX_TERROR_SPIRIT) +
                        TERROR_SPIRIT_PER_LEVEL *
                            passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SERVANT_OF_THE_RING));
        terrorFx(foe);
    }
    else if (before < DREAD_FRIGHTENED && mark->dread >= DREAD_FRIGHTENED)
    {
        frightFx(foe);
    }
}

static u32 maxThralls(u8 playerIndex)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_MORGUL_SORCERY);

    return THRALL_BASE_MAX + (level >= 2) + (level >= 4) +
           HOST_THRALLS_PER_LEVEL *
               passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HOST_OF_ANGMAR) +
           (hasUnique(playerIndex, WRAITH_UNIQUE_BANNER_OF_ANGMAR) ? BANNER_EXTRA_THRALLS : 0);
}

/** The wraith's next living thrall after cursor, NULL to start. */
static Actor *nextThrall(Actor *owner, Actor *cursor)
{
    return hackLib_nextOwnedGhost(owner, cursor);
}

/** Hit and add Dread to every foe within radius of center. */
static void burstAround(u8 playerIndex, Actor *center, s32 radius, s32 damage, s32 dread)
{
    Actor *playerActor = PLAYER(playerIndex).ownerActor;
    Actor *foe = NULL;

    while ((foe = actor_iterateInRadius(center, foe, radius << 16)) != NULL)
    {
        if (foe != center && skillExpansion_isFoe(foe))
        {
            if (damage != 0)
            {
                skillExpansion_dealDamage(playerActor, foe, damage, HACKLIB_DAMAGE_SECONDARY,
                                          HACKLIB_ABILITY_NONE);
            }
            addDread(playerIndex, foe, dread);
        }
    }
    hackLib_fxBurst(center, FX_SHAPE_POOL_FLASH, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    hackLib_fxBurst(center, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
}

/** Master of the Dead keeps thralls alive. Grave Pact bursts a thrall about to expire. */
static void tickThralls(u8 playerIndex, Actor *playerActor)
{
    bool32 master = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_MASTER_OF_THE_DEAD) != 0;
    s32 pact = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_GRAVE_PACT);
    Actor *thrall = NULL;

    if (!master && pact == 0)
    {
        return;
    }
    while ((thrall = nextThrall(playerActor, thrall)) != NULL)
    {
        GhostState *gs = ghostState(thrall);

        // lifetime 1 is a thrall consumed by Command of the Nine.
        if (master && gs->lifetime > 1 && gs->lifetime < THRALL_FRAMES)
        {
            gs->lifetime = THRALL_FRAMES;
        }
        // A countdown passes 2 exactly once whichever actor ticks first.
        else if (!master && pact != 0 && gs->lifetime == 2)
        {
            burstAround(playerIndex, thrall, COMMAND_RADIUS,
                        GRAVE_PACT_BASE_DAMAGE + GRAVE_PACT_DAMAGE_PER_LEVEL * pact,
                        GRAVE_PACT_DREAD);
        }
    }
}

/** A foe slain under a Morgul shard rises as a thrall of the shard's wraith. */
static void raiseThrall(DreadMark *mark)
{
    u8 p = mark->shardOwner;
    Actor *owner = PLAYER(p).ownerActor;
    s32 level = passiveLevel(p, SKILL_EXPANSION_PASSIVE_MORGUL_SORCERY);
    Actor *target = NULL;
    Actor *thrall;
    s32 damage;

    if (owner == NULL || !ringwraith_isHero(p) || owner->as.combat.comboState[0] >= maxThralls(p))
    {
        if (owner != NULL && ringwraith_isHero(p))
        {
            skillExpansion_feedback(p, "Thrall limit reached");
        }
        return;
    }
    do
    {
        target = findNextEnemy(owner, target);
    } while (target != NULL && (target == mark->actor || target->currentHp <= 0));
    thrall = spawnNpc(GHOST_NPC_KIND, &mark->actor->coords, owner, target,
                      THRALL_FRAMES + THRALL_FRAMES_PER_LEVEL * level +
                          GRAVE_PACT_FRAMES_PER_LEVEL *
                              passiveLevel(p, SKILL_EXPANSION_PASSIVE_GRAVE_PACT) +
                          gear(p, WRAITH_FX_THRALL_FRAMES));
    if (thrall == NULL)
    {
        return;
    }
    damage = THRALL_BASE_DAMAGE + THRALL_DAMAGE_PER_LEVEL * mark->shardLevel +
             gear(p, WRAITH_FX_THRALL_DAMAGE) +
             (hasUnique(p, WRAITH_UNIQUE_BANNER_OF_ANGMAR) ? BANNER_THRALL_DAMAGE : 0);
    ghostState(thrall)->damage = damage > 0xff ? 0xff : damage;
    thrall->tickCallback = ringwraith_tickThrall;
    skillExpansion_trackGhost(thrall);
    owner->as.combat.comboState[0]++;
    hackLib_fxBurst(thrall, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    hackLib_fxBurst(thrall, FX_SHAPE_RISING_STREAKS, FX_COLOR_BLUE, FX_LAYER_FRONT);
}

/** The wraith whose Will of Sauron is active, or -1. */
static s32 willOwner(void)
{
    u32 p;

    for (p = 0; p < 2; p++)
    {
        if (states[p].willFrames != 0 && ringwraith_isHero(p))
        {
            return p;
        }
    }
    return -1;
}

static void updateMarks(void)
{
    s32 will = willOwner();
    u32 i;

    if (marks == NULL)
    {
        return;
    }

    for (i = 0; i < DREAD_MARK_COUNT; i++)
    {
        DreadMark *mark = &marks[i];

        if (mark->actor == NULL)
        {
            continue;
        }
        if (mark->generation == 0 || hackLib_actorGeneration(mark->actor) != mark->generation ||
            mark->actor->unitType != mark->unitType)
        {
            mark->actor = NULL;
            continue;
        }
        if (mark->actor->currentHp <= 0)
        {
            if (mark->shardFrames == 0 && will >= 0)
            {
                u8 bladeLevel = *skillExpansion_activeLevel(will, ROW_MORGUL_BLADE);

                mark->shardFrames = 1;
                mark->shardOwner = will;
                mark->shardLevel = bladeLevel != 0 ? bladeLevel : 1;
            }
            if (mark->shardFrames != 0)
            {
                raiseThrall(mark);
            }
            mark->actor = NULL;
            continue;
        }
        if (mark->shardFrames != 0)
        {
            mark->shardFrames--;
        }
        if (mark->terrorFrames != 0)
        {
            mark->terrorFrames--;
        }
        if (mark->dread == 0 && mark->shardFrames == 0 && mark->terrorFrames == 0)
        {
            mark->actor = NULL;
        }
    }
}

/** Dread aura, Morgul shard burns and Black Breath drains, every PULSE_FRAMES. */
static void pulse(u8 playerIndex, Actor *playerActor)
{
    RingwraithState *st = &states[playerIndex];
    s32 radius =
        AURA_RADIUS + gear(playerIndex, WRAITH_FX_FEAR_REACH) +
        AURA_RADIUS_PER_LEVEL * passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FEARS_REACH);
    s32 auraDread =
        AURA_DREAD + DREAD_PER_HIT_PER_TERROR_LEVEL *
                         passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR);
    bool32 endlessNight = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ENDLESS_NIGHT) != 0;
    bool32 decay = !hasUnique(playerIndex, WRAITH_UNIQUE_IRON_CROWN) && !endlessNight;
    Actor *foe = NULL;
    u32 i;

    if (hasUnique(playerIndex, WRAITH_UNIQUE_IRON_CROWN))
    {
        radius *= 2;
    }
    if (endlessNight)
    {
        radius *= 2;
    }
    if (hasUnique(playerIndex, WRAITH_UNIQUE_VOICE_OF_SAURON))
    {
        radius /= 2;
        auraDread *= 2;
    }
    for (i = 0; marks != NULL && i < DREAD_MARK_COUNT && decay; i++)
    {
        if (marks[i].actor != NULL && marks[i].dread != 0)
        {
            marks[i].dread = marks[i].dread > DREAD_DECAY ? marks[i].dread - DREAD_DECAY : 0;
        }
    }
    while ((foe = actor_iterateInRadius(playerActor, foe, radius << 16)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            addDread(playerIndex, foe, auraDread);
        }
    }

    for (i = 0; marks != NULL && i < DREAD_MARK_COUNT; i++)
    {
        DreadMark *mark = &marks[i];

        if (mark->actor != NULL && mark->shardFrames != 0 && mark->shardOwner == playerIndex &&
            skillExpansion_isFoe(mark->actor))
        {
            skillExpansion_dealDamage(playerActor, mark->actor,
                                      SHARD_BASE_DAMAGE + mark->shardLevel, HACKLIB_DAMAGE_PERIODIC,
                                      ROW_MORGUL_BLADE);
            shardFx(mark->actor);
        }
    }

    if (hasUnique(playerIndex, WRAITH_UNIQUE_BARROW_CROWN))
    {
        Actor *thrall = NULL;

        while ((thrall = nextThrall(playerActor, thrall)) != NULL)
        {
            foe = NULL;
            while ((foe = actor_iterateInRadius(thrall, foe, BARROW_CROWN_RADIUS << 16)) != NULL)
            {
                if (skillExpansion_isFoe(foe))
                {
                    addDread(playerIndex, foe, BARROW_CROWN_DREAD);
                }
            }
        }
    }

    if (st->mordorFrames != 0)
    {
        hackLib_fxAt(playerActor, &st->mordorCenter, FX_SHAPE_OVAL_RING, FX_COLOR_NIGHT);
        foe = NULL;
        while ((foe = findNextEnemy(playerActor, foe)) != NULL)
        {
            s32 dx = (foe->coords.x - st->mordorCenter.x) >> 16;
            s32 dy = (foe->coords.y - st->mordorCenter.y) >> 16;

            if (!skillExpansion_isFoe(foe) || dx * dx + dy * dy > MORDOR_RADIUS * MORDOR_RADIUS)
            {
                continue;
            }
            addDread(playerIndex, foe, MORDOR_DREAD);
            if (dreadOf(foe) >= DREAD_FRIGHTENED)
            {
                skillExpansion_dealDamage(playerActor, foe,
                                          MORDOR_BASE_DAMAGE +
                                              MORDOR_DAMAGE_PER_LEVEL * st->mordorLevel,
                                          HACKLIB_DAMAGE_PERIODIC, ROW_SHADOW_OF_MORDOR);
            }
            gloomFx(foe);
        }
    }

    if (st->breathFrames != 0)
    {
        s32 drained = 0;

        foe = NULL;
        while ((foe = actor_iterateInRadius(playerActor, foe, BREATH_RADIUS << 16)) != NULL)
        {
            if (skillExpansion_isFoe(foe))
            {
                s32 damage = BREATH_BASE_DAMAGE + dreadOf(foe) / BREATH_DREAD_PER_DAMAGE;
                HackLibDamageResult result = skillExpansion_dealDamage(
                    playerActor, foe, damage, HACKLIB_DAMAGE_PERIODIC, ROW_BLACK_BREATH);

                drained += result.damageDealt;
                hackLib_fxBurst(foe, FX_SHAPE_SMOKE_SMALL, FX_COLOR_GREEN, FX_LAYER_FRONT);
            }
        }
        if (drained != 0)
        {
            hackLib_fxBurst(playerActor, FX_SHAPE_RING_SPARKLES, FX_COLOR_GREEN, FX_LAYER_FRONT);
        }
        healDrain(playerIndex, drained, DRAIN_HEAL_PERCENT);
    }
}

static void endUnseen(RingwraithState *st, Actor *playerActor)
{
    st->unseenFrames = 0;
    st->stepStrike = FALSE;
    playerActor->spriteFlags &= ~SPRITE_FLAG_STEALTH;
    playerActor->flags.p &= ~ACTOR_FLAG_23;
}

/** Become Unseen for at least frames. Endless Night forbids it. */
static bool32 enterUnseen(u8 playerIndex, Actor *playerActor, s32 frames)
{
    RingwraithState *st = &states[playerIndex];

    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ENDLESS_NIGHT) != 0)
    {
        return FALSE;
    }
    hackLib_refreshTimer(&st->unseenFrames, frames);
    playerActor->spriteFlags |= SPRITE_FLAG_STEALTH;
    playerActor->flags.p |= ACTOR_FLAG_23;
    hackLib_fxBurst(playerActor, FX_SHAPE_SMOKE_RISE, FX_COLOR_NIGHT, FX_LAYER_FRONT);
    hackLib_fxBurst(playerActor, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    sfx_play(UNSEEN_SFX);
    return TRUE;
}

/** The wraith starts each mission with a quarter of its Spirit, or all of it with Ringbound. */
static void limitStartSpirit(u8 playerIndex)
{
    s32 maxSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    s32 start = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RINGBOUND) != 0
                    ? maxSpirit
                    : percentOf(maxSpirit, START_SPIRIT_PERCENT);

    if (PLAYER(playerIndex).currentSpirit > start)
    {
        PLAYER(playerIndex).currentSpirit = start;
    }
}

/** Follow the sword's animation-frame hitbox with a white glint. */
static void tickBladeSpark(Actor *fx)
{
    Actor *owner = fx->parentActor;
    const struct ActorTriggerBox *blade;
    Vector2Fp16 pos;
    s32 x;
    s32 y;

    if (owner == NULL || !isWraithAvatar(owner) || (owner->flags.p & ACTOR_FLAG_PENDING_REMOVE))
    {
        hackLib_fxRemove(fx);
        return;
    }
    if (fx_tickAttachment(fx))
    {
        return;
    }
    blade = &owner->triggerBoxes[COMBAT_TRIGGER_HITBOX];
    x = (blade->xMin + blade->xMax) / 2;
    y = (blade->yMin + blade->yMax) / 2;
    if (!blade->enable)
    {
        x = 10;
        y = -16;
    }
    if (owner->oam.affineMode == OAM_AFFINE_OFF && (owner->oam.matrixIndex & OAM_MATRIX_H_FLIP))
    {
        x = -x;
    }
    pos.x = owner->coords.x + x * FP16_16_ONE;
    pos.y = owner->coords.y + y * FP16_16_ONE;
    hackLib_fxDetach(fx, &pos);
}

static void spawnBladeSpark(Actor *playerActor)
{
    Actor *fx = hackLib_fxBurst(playerActor, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);

    if (fx != NULL)
    {
        fx->tickCallback = tickBladeSpark;
        tickBladeSpark(fx);
    }
}

/** Tint the sword and renew its glints during Morgul Blade's afterglow. */
static void tickBladeGlow(RingwraithState *st, Actor *playerActor)
{
    u32 left = --st->bladeGlowFrames;
    u32 strength = left >= BLADE_GLOW_FRAMES - BLADE_GLOW_RISE_FRAMES
                       ? (BLADE_GLOW_FRAMES - left) * FX_TINT_FULL / BLADE_GLOW_RISE_FRAMES
                       : left * FX_TINT_FULL / (BLADE_GLOW_FRAMES - BLADE_GLOW_RISE_FRAMES);

    hackLib_fxTintSlot(playerActor->oam.paletteNumber, ObjPalettes[RINGWRAITH_OBJ_PALETTE],
                       BLADE_FIRST_COLOR, BLADE_COLOR_COUNT, BLADE_GLOW_COLOR, strength);
    if (left != 0 && left % BLADE_SPARK_INTERVAL == 0)
    {
        spawnBladeSpark(playerActor);
    }
}

void ringwraith_kitReset(u8 playerIndex)
{
    u32 i;

    memory_memset(&states[playerIndex], 0, sizeof(states[playerIndex]));
    states[playerIndex].lastSpirit = -1;
    if (!ringwraith_isHero(1 - playerIndex))
    {
        if (marks != NULL)
        {
            memory_free(marks);
            marks = NULL;
        }
    }
    else if (marks != NULL)
    {
        for (i = 0; i < DREAD_MARK_COUNT; i++)
        {
            if (marks[i].shardOwner == playerIndex)
            {
                marks[i].shardFrames = 0;
            }
        }
    }
}

static void cleanup(u8 playerIndex, Actor *actor)
{
    if (states[playerIndex].unseenFrames != 0)
    {
        endUnseen(&states[playerIndex], actor);
    }
}

void ringwraith_kitTick(u8 playerIndex, Actor *playerActor)
{
    RingwraithState *st = &states[playerIndex];

    if (!ringwraith_isHero(playerIndex))
    {
        return;
    }
    wraithGear_refresh(playerIndex);
    // Spirit rises only through grantSpirit, so any other rise is a mission start refill.
    if (st->lastSpirit < 0 || PLAYER(playerIndex).currentSpirit > st->lastSpirit)
    {
        limitStartSpirit(playerIndex);
    }

    updateMarks();
    tickThralls(playerIndex, playerActor);
    if (st->pulseFrames == 0)
    {
        st->pulseFrames = PULSE_FRAMES;
        pulse(playerIndex, playerActor);
    }
    st->pulseFrames--;

    if (hasUnique(playerIndex, WRAITH_UNIQUE_MORGUL_VALE) && st->valeFrames-- == 0)
    {
        Actor *foe = nearestFoe(playerActor, MORGUL_VALE_RANGE, 0);

        st->valeFrames = MORGUL_VALE_FRAMES;
        if (foe != NULL)
        {
            addDread(playerIndex, foe, MORGUL_VALE_DREAD);
            hackLib_fxBurst(foe, FX_SHAPE_LIGHTNING, FX_COLOR_MORGUL, FX_LAYER_FRONT);
        }
    }
    if (st->bladeGlowFrames != 0)
    {
        tickBladeGlow(st, playerActor);
    }
    if (st->breathFrames == 60 || st->maliceFrames == 60 || st->sacrificeFrames == 60 ||
        st->willFrames == 60 || st->mordorFrames == 60 || st->unseenFrames == 60)
    {
        const char *message = st->breathFrames == 60      ? "Black Breath fading"
                              : st->maliceFrames == 60    ? "No Living Man fading"
                              : st->sacrificeFrames == 60 ? "Black Sacrifice fading"
                              : st->willFrames == 60      ? "Will of Sauron fading"
                              : st->mordorFrames == 60    ? "Shadow of Mordor fading"
                                                          : "Unseen fading";
        skillExpansion_feedback(playerIndex, message);
    }
    if (st->breathFrames != 0)
    {
        st->breathFrames--;
    }
    if (st->maliceFrames != 0)
    {
        st->maliceFrames--;
    }
    if (st->sacrificeFrames != 0)
    {
        st->sacrificeFrames--;
    }
    if (st->willFrames != 0)
    {
        st->willFrames--;
    }
    if (st->mordorFrames != 0 && --st->mordorFrames == 0)
    {
        hackLib_fxAt(playerActor, &st->mordorCenter, FX_SHAPE_SMOKE_RISE, FX_COLOR_NIGHT);
    }
    if (st->unseenFrames != 0 && --st->unseenFrames == 0)
    {
        endUnseen(st, playerActor);
    }
    st->lastSpirit = PLAYER(playerIndex).currentSpirit;
}

/** Dread Lord bonus percent against a foe at dread, doubled against the terrified. */
static s32 dreadLordPercent(u8 playerIndex, u32 dread)
{
    if (dread < DREAD_FRIGHTENED)
    {
        return 0;
    }
    return DREAD_LORD_PERCENT_PER_LEVEL *
           passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DREAD_LORD) *
           (dread >= DREAD_MAX ? 2 : 1);
}

s32 ringwraith_kitMeleeDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    RingwraithState *st = &states[playerIndex];
    bool32 deathFromShadow;
    s32 level;
    u32 dread;

    if (!ringwraith_isHero(playerIndex))
    {
        return damage;
    }
    dread = dreadOf(victim);
    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WITCH_KINGS_MACE);
    if (dread >= DREAD_FRIGHTENED)
    {
        damage += percentOf(damage, MACE_PERCENT_PER_LEVEL * level * (dread >= DREAD_MAX ? 2 : 1) +
                                        gear(playerIndex, WRAITH_FX_FRIGHT_DAMAGE) +
                                        dreadLordPercent(playerIndex, dread));
    }
    st->drainPercent = gear(playerIndex, WRAITH_FX_LIFE_DRAIN);
    if (dread >= DREAD_MAX && hasUnique(playerIndex, WRAITH_UNIQUE_BLACK_CAPTAIN))
    {
        damage *= 2;
        st->drainPercent += BLACK_CAPTAIN_DRAIN_PERCENT;
    }
    if (hasUnique(playerIndex, WRAITH_UNIQUE_MORGUL_KNIFE))
    {
        damage -= percentOf(damage, MORGUL_KNIFE_PENALTY_PERCENT);
    }
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_MASTER_OF_THE_DEAD) != 0)
    {
        damage -= percentOf(damage, MASTER_DAMAGE_PENALTY_PERCENT);
    }
    deathFromShadow = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DEATH_FROM_SHADOW) != 0;
    if (st->unseenFrames != 0)
    {
        damage *=
            (hasUnique(playerIndex, WRAITH_UNIQUE_SHROUD_OF_UNBEING) ? SHROUD_UNSEEN_MULTIPLIER
                                                                     : UNSEEN_DAMAGE_MULTIPLIER) +
            (deathFromShadow ? DEATH_FROM_SHADOW_EXTRA_MULTIPLIER : 0);
        damage += percentOf(damage, gear(playerIndex, WRAITH_FX_UNSEEN_DAMAGE));
        st->unseenStrike = TRUE;
    }
    else if (deathFromShadow)
    {
        damage -= percentOf(damage, DEATH_FROM_SHADOW_PENALTY_PERCENT);
    }
    return damage;
}

s32 ringwraith_kitRangedDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    u32 dread;

    if (!ringwraith_isHero(playerIndex))
    {
        return damage;
    }
    dread = dreadOf(victim);
    if (dread >= DREAD_FRIGHTENED)
    {
        damage += percentOf(damage, dreadLordPercent(playerIndex, dread));
    }
    return damage;
}

void ringwraith_kitOnHit(u8 playerIndex, Actor *attacker, Actor *victim, bool32 killed,
                         bool32 melee, s32 damageDealt)
{
    RingwraithState *st = &states[playerIndex];
    u8 strikeShardLevel = 0;

    if (!ringwraith_isHero(playerIndex))
    {
        return;
    }
    if (melee && hasUnique(playerIndex, WRAITH_UNIQUE_MORGUL_KNIFE))
    {
        strikeShardLevel = 1;
    }
    if (melee && st->unseenStrike && hasUnique(playerIndex, WRAITH_UNIQUE_BLACK_DART))
    {
        u8 level = *skillExpansion_activeLevel(playerIndex, ROW_MORGUL_BLADE);

        strikeShardLevel = level != 0 ? level : 1;
    }
    if (melee && !killed)
    {
        if (dreadOf(victim) >= DREAD_FRIGHTENED)
        {
            grantSpirit(playerIndex, FRIGHTENED_HIT_SPIRIT);
        }
        addDread(playerIndex, victim,
                 DREAD_PER_HIT + gear(playerIndex, WRAITH_FX_DREAD_PER_HIT) +
                     DREAD_PER_HIT_PER_TERROR_LEVEL *
                         passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR));
        if (hasUnique(playerIndex, WRAITH_UNIQUE_MORGUL_KNIFE))
        {
            DreadMark *mark = getMark(victim);

            if (mark != NULL && mark->shardFrames == 0)
            {
                mark->shardFrames = SHARD_FRAMES;
                mark->shardOwner = playerIndex;
                mark->shardLevel = 1;
            }
        }
    }
    if (melee && hasUnique(playerIndex, WRAITH_UNIQUE_NINTH_RING) &&
        ++st->meleeHits >= NINTH_RING_HITS)
    {
        st->meleeHits = 0;
        addDread(playerIndex, victim, DREAD_MAX);
    }
    if (melee && st->drainPercent != 0)
    {
        healDrain(playerIndex, damageDealt, st->drainPercent);
        st->drainPercent = 0;
    }
    if (melee && st->unseenStrike)
    {
        bool32 stepStrike = st->stepStrike;
        s32 knife = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_KNIFE_IN_THE_DARK);

        st->unseenStrike = FALSE;
        endUnseen(st, attacker);
        if (!killed)
        {
            addDread(playerIndex, victim, DREAD_MAX);
        }
        if (hasUnique(playerIndex, WRAITH_UNIQUE_BLACK_DART))
        {
            DreadMark *mark = getMark(victim);
            u8 bladeLevel = *skillExpansion_activeLevel(playerIndex, ROW_MORGUL_BLADE);

            if (mark != NULL)
            {
                mark->shardFrames = SHARD_FRAMES;
                mark->shardOwner = playerIndex;
                mark->shardLevel = bladeLevel != 0 ? bladeLevel : 1;
            }
        }
        if (killed && knife != 0)
        {
            enterUnseen(playerIndex, attacker, KNIFE_BASE_FRAMES + KNIFE_FRAMES_PER_LEVEL * knife);
        }
        if (killed && stepStrike && hasUnique(playerIndex, WRAITH_UNIQUE_UNLIT_HAND))
        {
            grantSpirit(playerIndex, PLAYER_ACTIVE_SKILL(playerIndex, ROW_SHADOWSTEP).spiritCost);
        }
        hackLib_fxBurst(victim, FX_SHAPE_TENDRIL_STAR, FX_COLOR_BLOOD, FX_LAYER_FRONT);
        hackLib_fxBurst(victim, FX_SHAPE_STAR_BLOOM, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    }
    if (!killed)
    {
        return;
    }
    if (dreadOf(victim) >= DREAD_MAX)
    {
        s32 feast = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FEAST_OF_FEAR);

        if (feast != 0)
        {
            heal(playerIndex, percentOf(PLAYER_STAT(playerIndex, STAT_MAX_HP),
                                        FEAST_HEAL_PERCENT_PER_LEVEL * feast));
            grantSpirit(playerIndex, FEAST_SPIRIT_PER_LEVEL * feast);
        }
        if (hasUnique(playerIndex, WRAITH_UNIQUE_DOL_GULDUR_MASK) && st->maskChain < MASK_MAX_CHAIN)
        {
            st->maskChain++;
            burstAround(playerIndex, victim, MASK_RADIUS, MASK_DAMAGE, MASK_DREAD);
            st->maskChain--;
        }
    }
    if (hasUnique(playerIndex, WRAITH_UNIQUE_RING_OF_KHAMUL))
    {
        heal(playerIndex, percentOf(PLAYER_STAT(playerIndex, STAT_MAX_HP), KHAMUL_HEAL_PERCENT));
    }
    if (hasUnique(playerIndex, WRAITH_UNIQUE_WITCH_KINGS_MACE))
    {
        Actor *foe = NULL;

        while ((foe = actor_iterateInRadius(victim, foe, WITCH_KINGS_MACE_RADIUS << 16)) != NULL)
        {
            if (foe != victim && skillExpansion_isFoe(foe))
            {
                addDread(playerIndex, foe, WITCH_KINGS_MACE_DREAD);
            }
        }
        hackLib_fxBurst(victim, FX_SHAPE_SHOCKWAVE, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    }
    grantSpirit(playerIndex,
                KILL_SPIRIT + gear(playerIndex, WRAITH_FX_KILL_SPIRIT) +
                    KILL_SPIRIT_PER_LEVEL *
                        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SERVANT_OF_THE_RING));
    if (st->maliceFrames != 0)
    {
        s32 amount =
            percentOf(PLAYER_STAT(playerIndex, STAT_MAX_HP),
                      MALICE_HEAL_PERCENT +
                          MALICE_HEAL_PERCENT_PER_LEVEL *
                              passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_UNDYING_MALICE));

        heal(playerIndex, amount + percentOf(amount, gear(playerIndex, WRAITH_FX_DRAIN_HEAL)));
        grantSpirit(playerIndex, MALICE_KILL_SPIRIT);
    }
    {
        DreadMark *mark = findMark(victim);

        if (mark != NULL && mark->shardFrames != 0)
        {
            raiseThrall(mark);
            mark->actor = NULL;
        }
        else if (st->willFrames != 0 || strikeShardLevel != 0)
        {
            DreadMark corpse = {0};
            u8 level = *skillExpansion_activeLevel(playerIndex, ROW_MORGUL_BLADE);

            corpse.actor = victim;
            corpse.shardOwner = playerIndex;
            corpse.shardLevel = st->willFrames != 0 ? (level != 0 ? level : 1) : strikeShardLevel;
            raiseThrall(&corpse);
            if (mark != NULL)
            {
                mark->actor = NULL;
            }
        }
    }
}

/**
 * The wraith heals only through its drains. Its damage taken follows Wraith Form, its foes'
 * Dread and No Living Man.
 *
 * REPLACES (whole function): player_addHp.
 */
void ringwraith_addHp(u32 playerIndex, s32 delta, u32 flinchOnDamage)
{
    u8 p = playerIndex;
    s16 amount = delta;
    Actor *attacker = ringwraith_kitEnemyAttacker;
    s32 damage;
    s32 level;

    if (!ringwraith_isHero(p) || amount == 0)
    {
        skillExpansion_original_player_addHp(p, amount, flinchOnDamage);
        return;
    }
    if (amount > 0)
    {
        if (states[p].allowGain || PLAYER(p).currentHp <= 0)
        {
            skillExpansion_original_player_addHp(p, amount, flinchOnDamage);
        }
        return;
    }

    damage = -amount;
    if (states[p].unseenFrames != 0 && PLAYER(p).ownerActor != NULL &&
        !hasUnique(p, WRAITH_UNIQUE_SHROUD_OF_UNBEING))
    {
        endUnseen(&states[p], PLAYER(p).ownerActor);
    }
    level = passiveLevel(p, SKILL_EXPANSION_PASSIVE_WRAITH_FORM);
    if (attacker != NULL && !(EnemyTypeInfos[attacker->unitType].attackFlags.p & ENEMY_ATTACK_FIRE))
    {
        damage -= percentOf(damage, WRAITH_FORM_PERCENT_PER_LEVEL * level);
    }
    else if (level != 0)
    {
        damage += percentOf(damage, WRAITH_FORM_WEAKNESS_PERCENT);
    }
    if (attacker != NULL && dreadOf(attacker) >= DREAD_FRIGHTENED)
    {
        damage -=
            percentOf(damage, FRIGHTENED_PERCENT +
                                  FRIGHTENED_PERCENT_PER_LEVEL *
                                      passiveLevel(p, SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR));
    }
    if (hasUnique(p, WRAITH_UNIQUE_IRON_CROWN))
    {
        damage += percentOf(damage, IRON_CROWN_DAMAGE_TAKEN_PERCENT);
    }
    damage -= percentOf(damage, gear(p, WRAITH_FX_WARD));
    if (damage < 1)
    {
        damage = 1;
    }
    if (hasUnique(p, WRAITH_UNIQUE_BARROW_HAUBERK) && PLAYER(p).currentSpirit > 0)
    {
        s32 spirit = (damage + 1) / 2;

        if (spirit > PLAYER(p).currentSpirit)
        {
            spirit = PLAYER(p).currentSpirit;
        }
        ringwraith_original_player_addSpirit(p, -spirit);
        states[p].lastSpirit = PLAYER(p).currentSpirit;
        damage -= spirit * 2;
        if (damage <= 0)
        {
            return;
        }
    }
    if (states[p].maliceFrames != 0 && PLAYER(p).currentHp - damage < 1)
    {
        damage = PLAYER(p).currentHp > 1 ? PLAYER(p).currentHp - 1 : 0;
    }
    skillExpansion_original_player_addHp(p, -damage, flinchOnDamage);
}

/**
 * The wraith gains Spirit only from its kills.
 *
 * REPLACES (whole function): player_addSpirit.
 */
void ringwraith_addSpirit(u32 playerIndex, s32 delta)
{
    u8 p = playerIndex;
    s16 amount = delta;

    if (ringwraith_isHero(p) && amount > 0 && !states[p].allowGain)
    {
        return;
    }
    ringwraith_original_player_addSpirit(p, amount);
}

/** The nearest foe within range whose Dread is at least minDread, or NULL. */
static bool32 meetsDread(Actor *target, u32 minimum)
{
    return dreadOf(target) >= minimum;
}

static Actor *nearestFoe(Actor *playerActor, s32 range, u32 minDread)
{
    return skillExpansion_findTarget(playerActor, range, meetsDread, minDread);
}

static SkillExpansionCastResult morgulBlade(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    Actor *target = nearestFoe(playerActor, MORGUL_RANGE, 0);
    DreadMark *mark;

    states[p].bladeGlowFrames = BLADE_GLOW_FRAMES;
    spawnBladeSpark(playerActor);
    if (target == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    mark = getMark(target);
    if (mark != NULL)
    {
        mark->shardFrames = SHARD_FRAMES;
        mark->shardOwner = p;
        mark->shardLevel = level;
    }
    skillExpansion_applyPveRangedHit(playerActor, target,
                                     MORGUL_BASE_DAMAGE + MORGUL_DAMAGE_PER_LEVEL * level);
    addDread(p, target, DREAD_MAX);
    hackLib_fxBurst(target, FX_SHAPE_TENDRIL_STAR, FX_COLOR_MORGUL, FX_LAYER_FRONT);
    hackLib_fxBurst(target, FX_SHAPE_POOL_FLASH, FX_COLOR_MORGUL, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_HIT_IMPALE);
    return SKILL_CAST_SUCCESS;
}

static void cryOfTheNazgul(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 damage = CRY_BASE_DAMAGE + CRY_DAMAGE_PER_LEVEL * castLevel(p);
    Actor *foe = NULL;

    if (hasUnique(p, WRAITH_UNIQUE_MORGUL_HORN))
    {
        damage *= HORN_DAMAGE_MULTIPLIER;
        grantSpirit(p, percentOf(PLAYER_ACTIVE_SKILL(p, ROW_CRY_OF_THE_NAZGUL).spiritCost,
                                 HORN_REFUND_PERCENT));
    }
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_OVAL_RING, FX_COLOR_GHOST, FX_LAYER_FRONT);
    sfx_play(EnemySfxSets[ENEMY_TYPE_NAZGUL].aggroSfx);
    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            skillExpansion_applyPveRangedHit(playerActor, foe, damage);
            addDread(p, foe, DREAD_MAX);
        }
    }
}

static void unseenWorld(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 fading = passiveLevel(p, SKILL_EXPANSION_PASSIVE_FADING);
    s32 frames = UNSEEN_BASE_FRAMES + UNSEEN_FRAMES_PER_LEVEL * castLevel(p) +
                 FADING_FRAMES_PER_LEVEL * fading + gear(p, WRAITH_FX_UNSEEN);
    s32 refundPercent = FADING_REFUND_PERCENT_PER_LEVEL * fading;

    if (hasUnique(p, WRAITH_UNIQUE_SHROUD_OF_UNBEING))
    {
        frames /= 2;
    }
    if (hasUnique(p, WRAITH_UNIQUE_DEAD_MARSHES))
    {
        frames += percentOf(frames, DEAD_MARSHES_UNSEEN_PERCENT);
    }
    enterUnseen(p, playerActor, frames);
    if (passiveLevel(p, SKILL_EXPANSION_PASSIVE_RINGBOUND) != 0)
    {
        refundPercent += RINGBOUND_REFUND_PERCENT;
    }
    if (refundPercent != 0)
    {
        grantSpirit(p,
                    percentOf(PLAYER_ACTIVE_SKILL(p, ROW_UNSEEN_WORLD).spiritCost, refundPercent));
    }
}

static void fellBeast(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 damage = FELL_BEAST_BASE_DAMAGE + FELL_BEAST_DAMAGE_PER_LEVEL * castLevel(p);
    bool32 saddle = hasUnique(p, WRAITH_UNIQUE_FELL_BEAST_SADDLE);
    s32 band = saddle ? FELL_BEAST_BAND * SADDLE_BAND_MULTIPLIER : FELL_BEAST_BAND;
    Actor *foe = NULL;

    sfx_play(EnemySfxSets[ENEMY_TYPE_NAZGUL].attackSfx);
    castFx(playerActor, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        s32 dy = (foe->coords.y - playerActor->coords.y) >> 16;

        if (skillExpansion_isFoe(foe) && dy <= band && dy >= -band)
        {
            skillExpansion_applyPveRangedHit(playerActor, foe, damage);
            addDread(p, foe, saddle ? DREAD_MAX : FELL_BEAST_DREAD);
            stun(foe, FELL_BEAST_STUN_FRAMES);
            hackLib_fxBurst(foe, FX_SHAPE_STREAK_RAIN, FX_COLOR_NIGHT, FX_LAYER_FRONT);
        }
    }
}

static void noLivingMan(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    states[p].maliceFrames =
        MALICE_BASE_FRAMES + MALICE_FRAMES_PER_LEVEL * castLevel(p) +
        MALICE_FRAMES_PER_MALICE_LEVEL * passiveLevel(p, SKILL_EXPANSION_PASSIVE_UNDYING_MALICE);
    hackLib_fxTimed(playerActor, FX_SHAPE_DIAMOND_RING, FX_COLOR_BLOOD, states[p].maliceFrames,
                    FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_POOL_FLASH, FX_COLOR_BLOOD, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
}

static void blackBreath(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;

    states[p].breathFrames = BREATH_FRAMES;
    states[p].pulseFrames = 0;
    castFxRaised(playerActor, FX_SHAPE_SMOKE_RISE, FX_COLOR_GREEN, FX_LAYER_FRONT,
                 BREATH_SMOKE_LIFT);
    castFx(playerActor, FX_SHAPE_GROUND_POOL, FX_COLOR_MORGUL, FX_LAYER_BEHIND);
    sfx_play(EnemySfxSets[ENEMY_TYPE_NAZGUL].hitSfx);
}

static void commandOfTheNine(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 damage = COMMAND_BASE_DAMAGE + COMMAND_DAMAGE_PER_LEVEL * castLevel(p);
    bool32 spareOne = hasUnique(p, WRAITH_UNIQUE_CARN_DUM_SIGNET);
    Actor *thrall = NULL;
    s32 count = 0;

    while ((thrall = nextThrall(playerActor, thrall)) != NULL)
    {
        if (count < COMMAND_FIRE_THRALLS)
        {
            Actor *fx = hackLib_fxEnemyAnim(thrall, ENEMY_TYPE_NAZGUL, ENEMY_ANIMATION_POSE_DEATH,
                                            NAZGUL_FIRE_TICKS, FX_LAYER_FRONT);

            if (fx != NULL)
            {
                hackLib_fxDetach(fx, &thrall->coords);
            }
        }
        burstAround(p, thrall, COMMAND_RADIUS, damage, COMMAND_DREAD);
        if (!spareOne || count != 0)
        {
            ghostState(thrall)->lifetime = 1;
        }
        count++;
    }
    grantSpirit(p, COMMAND_SPIRIT_PER_THRALL * count);
    sfx_play(EnemySfxSets[ENEMY_TYPE_NAZGUL].deathSfx);
}

static SkillExpansionCastResult shadowstep(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    Actor *target = nearestFoe(playerActor, SHADOWSTEP_RANGE, DREAD_FRIGHTENED);
    Actor *departure;
    s32 side;
    Vector2Fp16 landing;
    Vector2Fp16 preferred;

    if (target == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    side = playerActor->coords.x < target->coords.x ? 1 : -1;
    preferred.x = target->coords.x + side * (SHADOWSTEP_OFFSET << 16);
    preferred.y = target->coords.y;
    if (!hackLib_findLanding(playerActor, &preferred, &landing))
    {
        return SKILL_CAST_BLOCKED_LANDING;
    }
    // The wraith's shape burns away in Nazgul fire where it stood.
    departure = hackLib_fxEnemyAnim(playerActor, ENEMY_TYPE_NAZGUL, ENEMY_ANIMATION_POSE_DEATH,
                                    NAZGUL_FIRE_TICKS, FX_LAYER_FRONT);
    if (departure != NULL)
    {
        hackLib_fxDetach(departure, &playerActor->coords);
    }
    playerActor->renderCoords.x += landing.x - playerActor->coords.x;
    playerActor->renderCoords.y += landing.y - playerActor->coords.y;
    playerActor->coords = landing;
    playerActor->velocity.x = playerActor->velocity.y = 0;
    playerActor->facing = (Direction8)actor_direction8Toward(playerActor, target);
    if (enterUnseen(p, playerActor,
                    SHADOWSTEP_BASE_FRAMES + SHADOWSTEP_FRAMES_PER_LEVEL * castLevel(p)))
    {
        states[p].stepStrike = TRUE;
    }
    hackLib_fxBurst(target, FX_SHAPE_SPARK, FX_COLOR_BLOOD, FX_LAYER_FRONT);
    return SKILL_CAST_SUCCESS;
}

static void shadowOfMordor(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    RingwraithState *st = &states[p];
    Actor *pool;

    st->mordorLevel = castLevel(p);
    st->mordorFrames = MORDOR_BASE_FRAMES + MORDOR_FRAMES_PER_LEVEL * st->mordorLevel;
    st->mordorCenter = playerActor->coords;
    st->pulseFrames = 0;
    // A pool of shadow spreading from where the wraith stands.
    pool = hackLib_fxSprite(playerActor, &HackLibFxShadowPool, FX_COLOR_SHADOW, MORDOR_SHADE_TICKS,
                            FX_LAYER_BEHIND);
    if (pool != NULL)
    {
        hackLib_fxDetach(pool, &st->mordorCenter);
    }
    else
    {
        pool = fxAtOr(playerActor, &st->mordorCenter, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW,
                      FX_COLOR_NIGHT);
    }
    hackLib_fxHold(pool, st->mordorFrames);
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    sfx_play(EnemySfxSets[ENEMY_TYPE_WITCH_KING].deathSfx);
}

static void blackSacrifice(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    s32 cost = percentOf(PLAYER(p).currentHp, SACRIFICE_HP_PERCENT);

    // The original skips Wraith Form, ward and Unseen breaking, so the price is exact.
    if (cost > 0)
    {
        skillExpansion_original_player_addHp(p, -cost, FALSE);
    }
    grantSpirit(p, SACRIFICE_BASE_SPIRIT + SACRIFICE_SPIRIT_PER_LEVEL * castLevel(p));
    states[p].sacrificeFrames = SACRIFICE_FRAMES;
    castFx(playerActor, FX_SHAPE_LIGHTNING, FX_COLOR_BLOOD, FX_LAYER_FRONT);
    hackLib_fxTimed(playerActor, FX_SHAPE_RISING_STREAKS, FX_COLOR_BLOOD, SACRIFICE_FRAMES,
                    FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
}

static void willOfSauron(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    Actor *foe = NULL;

    states[p].willFrames = WILL_BASE_FRAMES + WILL_FRAMES_PER_LEVEL * castLevel(p);
    // The Eye: a fiery core with lashing tendrils and a widening ring of flame.
    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_EMBER, FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_SHOCKWAVE, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    sfx_play(EnemySfxSets[ENEMY_TYPE_NAZGUL].aggroSfx);
    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            addDread(p, foe, DREAD_MAX);
        }
    }
}

static bool32 canCastCommandOfTheNine(u8 playerIndex)
{
    Actor *playerActor = PLAYER(playerIndex).ownerActor;

    return playerActor != NULL && nextThrall(playerActor, NULL) != NULL;
}

static bool32 canCastShadowstep(u8 playerIndex)
{
    Actor *playerActor = PLAYER(playerIndex).ownerActor;

    return playerActor != NULL &&
           nearestFoe(playerActor, SHADOWSTEP_RANGE, DREAD_FRIGHTENED) != NULL;
}

static bool32 canCastShadowOfMordor(u8 playerIndex)
{
    return states[playerIndex].mordorFrames == 0;
}

static bool32 canCastBlackSacrifice(u8 playerIndex)
{
    return states[playerIndex].sacrificeFrames == 0 && PLAYER(playerIndex).currentHp > 1;
}

static bool32 canCastWillOfSauron(u8 playerIndex)
{
    return states[playerIndex].willFrames == 0;
}

static bool32 canCast(u8 playerIndex, u32 row)
{
    RingwraithState *st = &states[playerIndex];
    Actor *playerActor = PLAYER(playerIndex).ownerActor;

    if (playerActor == NULL)
    {
        return FALSE;
    }
    switch (row)
    {
    case ROW_MORGUL_BLADE:
        return nearestFoe(playerActor, MORGUL_RANGE, 0) != NULL;
    case ROW_UNSEEN_WORLD:
        return st->unseenFrames == 0 &&
               passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ENDLESS_NIGHT) == 0;
    case ROW_NO_LIVING_MAN:
        return st->maliceFrames == 0;
    case ROW_BLACK_BREATH:
        return st->breathFrames == 0;
    default:
        return TRUE;
    }
}

#define CAST_FIELDS(triggerFrame)                                                                  \
    .field_0x20 = 2, .field_0x22 = 5, .castTriggerFrame = (triggerFrame), .field_0x24 = 0xff

static const ActiveSkill RingwraithActiveSkills[HERO_ACTIVE_SKILL_COUNT] = {
    [ROW_MORGUL_BLADE] = {.spiritCost = 20, CAST_FIELDS(4)},
    [ROW_CRY_OF_THE_NAZGUL] = {.callback = cryOfTheNazgul, .spiritCost = 45, CAST_FIELDS(4)},
    [ROW_UNSEEN_WORLD] = {.callback = unseenWorld, .spiritCost = 30, CAST_FIELDS(2)},
    [ROW_FELL_BEAST] = {.callback = fellBeast, .spiritCost = 60, CAST_FIELDS(4)},
    [ROW_NO_LIVING_MAN] = {.callback = noLivingMan, .spiritCost = 80, CAST_FIELDS(4)},
    [ROW_BLACK_BREATH] = {.callback = blackBreath, .spiritCost = 15, CAST_FIELDS(0)},
};

static const void *const RingwraithActiveIcons[HERO_ACTIVE_SKILL_COUNT] = {
    [ROW_MORGUL_BLADE] = RingwraithIconMorgulBlade,
    [ROW_CRY_OF_THE_NAZGUL] = RingwraithIconCryOfTheNazgul,
    [ROW_UNSEEN_WORLD] = RingwraithIconUnseenWorld,
    [ROW_FELL_BEAST] = RingwraithIconFellBeast,
    [ROW_NO_LIVING_MAN] = RingwraithIconNoLivingMan,
    [ROW_BLACK_BREATH] = RingwraithIconBlackBreath,
};

static const s8 RingwraithActiveRequiredLevels[HERO_ACTIVE_SKILL_COUNT]
                                              [SKILL_EXPANSION_REQUIRED_LEVEL_COUNT] = {
                                                  [ROW_MORGUL_BLADE] = {1, 3, 5, 7, 9, -1},
                                                  [ROW_CRY_OF_THE_NAZGUL] = {3, 6, 9, 12, 15, -1},
                                                  [ROW_UNSEEN_WORLD] = {2, 4, 6, 8, 10, -1},
                                                  [ROW_FELL_BEAST] = {8, 11, 14, 17, 20, -1},
                                                  [ROW_NO_LIVING_MAN] = {12, 14, 16, 18, 20, -1},
                                                  [ROW_BLACK_BREATH] = {-1, -1, -1, -1, -1, -1},
};

static const char *const RingwraithActiveDescriptions[HERO_ACTIVE_SKILL_COUNT] = {
    [ROW_MORGUL_BLADE] = "Morgul Blade (Level @1) - Stab a foe for 8 +4/level. The shard burns "
                         "and fills its Dread. Slain, it rises as your thrall for 10s. @2 Spirit.",
    [ROW_CRY_OF_THE_NAZGUL] = "Cry of the Nazgul (Level @1) - A shriek terrifies every foe in "
                              "sight, freezing them and dealing 2 +2/level damage. @2 Spirit.",
    [ROW_UNSEEN_WORLD] = "The Unseen World (Level @1) - Vanish for 2.5s +0.5s/level. Your first "
                         "blow deals triple damage and terrifies. A hit breaks it. @2 Spirit.",
    [ROW_FELL_BEAST] = "Fell Beast (Level @1) - Your steed dives across the screen, striking every "
                       "foe in its path for 14 +7/level and adding Dread. @2 Spirit.",
    [ROW_NO_LIVING_MAN] = "No Living Man (Level @1) - For 3s +0.75s/level you cannot fall. Kills "
                          "heal 8% of your health and feed you Spirit. @2 Spirit.",
    [ROW_BLACK_BREATH] = "Black Breath - For 6s your breath drains nearby foes and heals you. The "
                         "more Dread they feel, the more you drain. @2 Spirit.",
};

static const u8 RingwraithPassiveSkillIds[HERO_PASSIVE_SKILL_COUNT] = {
    SKILL_EXPANSION_PASSIVE_SERVANT_OF_THE_RING, SKILL_EXPANSION_PASSIVE_SHADOW_HUNGER,
    SKILL_EXPANSION_PASSIVE_WRAITH_FORM,         SKILL_EXPANSION_PASSIVE_FEARS_REACH,
    SKILL_EXPANSION_PASSIVE_LORD_OF_TERROR,      SKILL_EXPANSION_PASSIVE_WITCH_KINGS_MACE,
    SKILL_EXPANSION_PASSIVE_MORGUL_SORCERY,      SKILL_EXPANSION_PASSIVE_UNDYING_MALICE,
    SKILL_EXPANSION_PASSIVE_RINGBOUND,
};

static void onHit(u8 p, Actor *attacker, Actor *victim, HackLibDamageResult result, bool melee)
{
    ringwraith_kitOnHit(p, attacker, victim, result.killed, melee, result.damageDealt);
}

const SkillExpansionHeroKit RingwraithHeroKit = {
    .activeSkills = RingwraithActiveSkills,
    .icons = RingwraithActiveIcons,
    .requiredLevels = RingwraithActiveRequiredLevels,
    .descriptions = RingwraithActiveDescriptions,
    .canCast = canCast,
    .passiveSkillIds = RingwraithPassiveSkillIds,
    .artHeroId = RINGWRAITH_BASE_HERO,
    .reset = ringwraith_kitReset,
    .cleanup = cleanup,
    .tick = ringwraith_kitTick,
    .refundSpirit = grantSpirit,
    .newActives = {&RingwraithCommandOfTheNine, &RingwraithShadowstep, &RingwraithShadowOfMordor,
                   &RingwraithBlackSacrifice, &RingwraithWillOfSauron},
    .newPassiveSkillIds = {SKILL_EXPANSION_PASSIVE_FADING, SKILL_EXPANSION_PASSIVE_HOST_OF_ANGMAR,
                           SKILL_EXPANSION_PASSIVE_DREAD_LORD,
                           SKILL_EXPANSION_PASSIVE_FEAST_OF_FEAR,
                           SKILL_EXPANSION_PASSIVE_GRAVE_PACT,
                           SKILL_EXPANSION_PASSIVE_KNIFE_IN_THE_DARK,
                           SKILL_EXPANSION_PASSIVE_MASTER_OF_THE_DEAD,
                           SKILL_EXPANSION_PASSIVE_DEATH_FROM_SHADOW,
                           SKILL_EXPANSION_PASSIVE_ENDLESS_NIGHT},
    .onHpChange = ringwraith_addHp,
    .resolvers = {[ROW_MORGUL_BLADE] = morgulBlade},
    .meleeDamage = ringwraith_kitMeleeDamage,
    .rangedDamage = ringwraith_kitRangedDamage,
    .onHit = onHit,
};

// Page 2. Each baseRow lends its cast pose and cooldown bit, so each uses a different page 1 row.
static const ActiveSkill CommandOfTheNineRow = {
    .callback = commandOfTheNine, .spiritCost = 35, CAST_FIELDS(4)};
static const s8 CommandOfTheNineLevels[] = {16, 19, 22, 25, 28, -1};
const SkillExpansionActiveDef RingwraithCommandOfTheNine = {
    .row = &CommandOfTheNineRow,
    .icon = RingwraithIconCommandOfTheNine,
    .requiredLevels = CommandOfTheNineLevels,
    .description = "Command of the Nine (Level @1) - Every thrall bursts for 12 +6/level damage "
                   "and Dread, then falls. +8 Spirit per thrall. @2 Spirit.",
    .canCast = canCastCommandOfTheNine,
    .baseRow = ROW_CRY_OF_THE_NAZGUL,
};

static const ActiveSkill ShadowstepRow = {.spiritCost = 25, CAST_FIELDS(2)};
static const s8 ShadowstepLevels[] = {15, 18, 21, 24, 27, -1};
const SkillExpansionActiveDef RingwraithShadowstep = {
    .resolve = shadowstep,
    .row = &ShadowstepRow,
    .icon = RingwraithIconShadowstep,
    .requiredLevels = ShadowstepLevels,
    .description = "Shadowstep (Level @1) - Step behind the nearest frightened foe and vanish for "
                   "0.75s +0.25s/level. Your next blow is Unseen. @2 Spirit.",
    .canCast = canCastShadowstep,
    .baseRow = ROW_UNSEEN_WORLD,
};

static const ActiveSkill ShadowOfMordorRow = {
    .callback = shadowOfMordor, .spiritCost = 50, CAST_FIELDS(4)};
static const s8 ShadowOfMordorLevels[] = {17, 20, 23, 26, 29, -1};
const SkillExpansionActiveDef RingwraithShadowOfMordor = {
    .row = &ShadowOfMordorRow,
    .icon = RingwraithIconShadowOfMordor,
    .requiredLevels = ShadowOfMordorLevels,
    .description = "Shadow of Mordor (Level @1) - Darken the ground here for 4s +1s/level. Foes on "
                   "it gain Dread. The frightened take 2 +2/level. @2 Spirit.",
    .canCast = canCastShadowOfMordor,
    .baseRow = ROW_FELL_BEAST,
};

static const ActiveSkill BlackSacrificeRow = {
    .callback = blackSacrifice, .spiritCost = 0, CAST_FIELDS(0)};
static const s8 BlackSacrificeLevels[] = {18, 21, 24, 27, 30, -1};
const SkillExpansionActiveDef RingwraithBlackSacrifice = {
    .row = &BlackSacrificeRow,
    .icon = RingwraithIconBlackSacrifice,
    .requiredLevels = BlackSacrificeLevels,
    .description = "Black Sacrifice (Level @1) - Pay a quarter of your health for 30 +10/level "
                   "Spirit. For 5s your drains heal double.",
    .canCast = canCastBlackSacrifice,
    .baseRow = ROW_MORGUL_BLADE,
};

static const ActiveSkill WillOfSauronRow = {
    .callback = willOfSauron, .spiritCost = 120, CAST_FIELDS(4)};
static const s8 WillOfSauronLevels[] = {32, 34, 36, 38, 40, -1};
const SkillExpansionActiveDef RingwraithWillOfSauron = {
    .row = &WillOfSauronRow,
    .icon = RingwraithIconWillOfSauron,
    .requiredLevels = WillOfSauronLevels,
    .description = "Will of Sauron (Level @1) - Terrify every foe in sight. For 10s +1s/level "
                   "every kill rises as a thrall, up to your limit. @2 Spirit.",
    .canCast = canCastWillOfSauron,
    .baseRow = ROW_NO_LIVING_MAN,
};

#endif
