#include "lib/fx_kit.h"
#include "lib/timed_effect.h"
#include "patches/ringwraith/saruman_icons.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "stats.h"
#include "variables.h"

#define SARUMAN_PARTS 8
#define SARUMAN_POSES 7
#define SARUMAN_BANK(a, b, c, d, e, f, g)                                                          \
    a, b, c, d, e, f, g, a, b, c, d, e, f, g, a, b, c, d, e, f, g, a, b, c, d, e, f, g, a, b, c,   \
        d, e, f, g, a, b, c, d, e, f, g, a, b, c, d, e, f, g, a, b, c, d, e, f, g
#define SARUMAN_STAND EnemyAnimationBank_EnemyTypeSaruman_Stand
#define SARUMAN_WALK EnemyAnimationBank_EnemyTypeSaruman_Walk
#define SARUMAN_HURT EnemyAnimationBank_EnemyTypeSaruman_Stagger
#define SARUMAN_ATTACK SarumanQuickAttackAnimations
#define SARUMAN_DEATH EnemyAnimationBank_EnemyTypeSaruman_Death

#define QUICK_ATTACK(facing)                                                                       \
    {EnemyAnimationTiles_EnemyTypeSaruman_Attack_Facing##facing,                                   \
     &EnemyAnimationFrames_EnemyTypeSaruman_Attack_Facing##facing.header, NULL, 1}
static const SpriteAnimation SarumanQuickAttackAnimations[5] = {
    QUICK_ATTACK(0), QUICK_ATTACK(1), QUICK_ATTACK(2), QUICK_ATTACK(3), QUICK_ATTACK(4)};

const SpriteAnimation *const SarumanMoveAnimationBank[SARUMAN_PARTS * SARUMAN_POSES] = {
    SARUMAN_BANK(SARUMAN_HURT, SARUMAN_DEATH, SARUMAN_HURT, SARUMAN_HURT, SARUMAN_STAND,
                 SARUMAN_HURT, SARUMAN_WALK)};
const SpriteAnimation *const SarumanMeleeAnimationBank[SARUMAN_PARTS * SARUMAN_POSES] = {
    SARUMAN_BANK(SARUMAN_ATTACK, SARUMAN_ATTACK, SARUMAN_ATTACK, SARUMAN_ATTACK, SARUMAN_ATTACK,
                 SARUMAN_ATTACK, SARUMAN_ATTACK)};
const SpriteAnimation *const SarumanCastAnimationBank[SARUMAN_PARTS * SARUMAN_POSES] = {
    SARUMAN_BANK(SARUMAN_ATTACK, SARUMAN_ATTACK, SARUMAN_ATTACK, SARUMAN_ATTACK, SARUMAN_ATTACK,
                 SARUMAN_ATTACK, SARUMAN_ATTACK)};

const HeroBaseStats SarumanBaseStats = {
    .courage = 36,
    .defense = 18,
    .health = 20,
    .accuracy = 32,
    .strength = 18,
};

#if HACK_skill_expansion

#include "fx.h"
#include "lib/actor_ref.h"
#include "lib/combat_source.h"
#include "lib/hud_meter.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "sfx.h"
#include "system.h"
#include "variables.h"

// heroes.json id
#define HERO_ID_SARUMAN 9

enum SarumanRow
{
    SARUMAN_VOICE = 0,
    SARUMAN_FLAME = 1,
    SARUMAN_BLASTING_FIRE = 2,
    SARUMAN_GUARD = 3,
    SARUMAN_BINDING = 4,
    SARUMAN_HEAL = 5,
    SARUMAN_ECHO = 7,
    SARUMAN_FURNACE = 8,
    SARUMAN_CURSE = 9,
    SARUMAN_LIGHT_BROKEN = 10,
    SARUMAN_SPITE = 11,
    SARUMAN_WHITE_HAND = 12,
};

#define EMPOWER_CAST_COUNT 3
#define EMPOWER_DAMAGE_PERCENT 50
#define EMPOWER_POWER_PER_LEVEL 15
#define COUNCIL_SPIRIT 6
#define REGEN_FRAMES 30
#define STAFF_HIT_SPIRIT 2
#define REMEDY_SPIRIT_CAP 8
#define ROW_NONE 0xff

#define RING_LORE_SPIRIT 2

#define BIND_PERCENT 25
#define CURSE_PERCENT 20
#define CURSE_PERCENT_PER_LEVEL 5
#define CURSE_LEAK_SPIRIT 1
#define CURSE_PULSE_FRAMES 30
#define CUTPURSE_SPIRIT 3
#define CUTPURSE_RANGE 96
#define SPITE_HP_PERCENT 40
#define SPITE_PERCENT_PER_LEVEL 6
#define SPITE_RADIUS 72
#define SPITE_STEAL_SPIRIT 2
#define SPITE_SPIRIT_CAP 8
#define PARTING_HP_PERCENT 30
#define PARTING_COOLDOWN_FRAMES 1200
#define MACHINE_SPIRIT 1
#define POWDER_RADIUS 40
#define POWDER_DAMAGE 20
#define FURNACE_TARGET_MAX 5
#define FIRES_DAMAGE_PER_LEVEL 1
#define FIRES_FRAMES_PER_LEVEL 60
#define ROBES_PERCENT_PER_CHARGE 2
#define KEEPER_PERCENT_PER_LEVEL 10
#define KINDLING_PERCENT_PER_LEVEL 6
#define WILDFIRE_RADIUS 64
#define WILDFIRE_FRAMES 90
#define WILDFIRE_FRAMES_PER_LEVEL 30
#define STAFF_BURN_FRAMES 90
#define HONEYED_PERCENT_PER_LEVEL 10
#define COMMAND_PERCENT_PER_LEVEL 6
#define HUD_PULSE_SHIFT 3

#define SARUMAN_BURN_ID 0xa01
#define SARUMAN_GUARD_ID 0xa02

#define CUE_COUNT 7

/** A spell bolt's trail flying from Saruman to where its target stood. */
typedef struct SarumanBolt
{
    s16 fromX;
    s16 fromY;
    s16 toX;
    s16 toY;

    // frames flown
    u8 frame;

    // SarumanFxId spawned along the path, FXID_NONE when no bolt flies
    u8 trail;
} SarumanBolt;

/** An effect waiting to play at a world position. */
typedef struct SarumanCue
{
    s16 x;
    s16 y;

    // frames left before it plays
    u8 delay;

    // SarumanFxId, FXID_NONE for a free cue
    u8 fx;
} SarumanCue;

typedef struct SarumanState
{
    SarumanBolt bolt;
    SarumanCue cues[CUE_COUNT];

    // Sharkey's Curse target
    HackLibActorRef curse;

    // Word of Binding target
    HackLibActorRef bound;

    // Sharkey's Curse frames left
    u16 curseFrames;

    // Word of Binding frames left
    u16 boundFrames;

    // frames until Parting Shot can fire again
    u16 partingCooldown;

    // where a burning foe died, for Wildfire to leap from next tick
    s16 wildfireX;
    s16 wildfireY;

    // Successful casts stored toward the empowered spell, capped at EMPOWER_CAST_COUNT.
    u8 spellCharges;

    // Last row Echo of Power can repeat, or ROW_NONE.
    u8 lastRow;

    // Sharkey's Curse cast level
    u8 curseLevel;

    // frames toward the next Wizard's Reserve point
    u8 regenFrames;

    // TRUE while HP was below the Parting Shot threshold at the last tick
    u8 wasLow;

    // TRUE while wildfireX/Y hold a leap to make
    u8 wildfirePending;
} SarumanState;

_Static_assert(sizeof(SarumanState) <= sizeof(skillExpansion_heroState[0]),
               "SarumanState exceeds the hero state");

static inline SarumanState *state(u8 playerIndex)
{
    return (SarumanState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_SARUMAN,
                                                         PLAYER(playerIndex).heroId);
}

static inline bool32 isSaruman(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_SARUMAN;
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline s32 scale(s32 value, s32 percent)
{
    return value * percent / 100;
}

#define CAST_FX_TICKS 3
#define SLOW_FX_TICKS 4
#define SMOKE_LIFT 14
#define BOLT_LIFT 12
#define GEM_LIFT 30
#define BOLT_FRAMES 12
#define BOLT_TRAIL_STEP 2
#define FOE_MARK_MAX 4
// the Saruman boss's attack sound
#define SARUMAN_CAST_SFX 0xdc
// the Saruman boss's aggro sound
#define SARUMAN_VOICE_SFX 0xdb

/** Effects Saruman leaves at a world position, now or from a cue. */
enum SarumanFxId
{
    FXID_NONE,
    FXID_FLAME_TRAIL,
    FXID_FUSE_TRAIL,
    FXID_WISP_TRAIL,
    FXID_FLAME_HIT,
    FXID_FLAME_LICK,
    FXID_BLAST_WAVE,
    FXID_BLAST_SMOKE,
    FXID_BLAST_FLASH,
    FXID_WISP_CLOUD,
    FXID_WISP_POOL,
    FXID_RIPPLE,
    FXID_RIPPLE_FAR,
    FXID_DAZE,
    FXID_SEAL,
    FXID_REMEDY_MOTE,
    FXID_REMEDY_GLOW,
    FXID_FURNACE_GLOW,
    FXID_FURNACE_ERUPT,
    FXID_PRISM_WHITE,
    FXID_PRISM_ISENGARD,
    FXID_PRISM_VOICE,
    FXID_STORM_BOLT,
    FXID_STORM_WAVE,
    FXID_FIRE_WAVE,
    FXID_SPITE_BLOOD,
    FXID_SPITE_DRAIN,
    FXID_POWDER_FLASH,
    FXID_POWDER_SMOKE,
    FXID_FURNACE_SMOKE,
    FXID_GEM_WHITE,
    FXID_GEM_ISENGARD,
    FXID_GEM_VOICE,
    FXID_DAZE_ORBIT,
    FXID_ECHO_RING,
    FXID_ECHO_REPEAT,
    FXID_SPITE_ARCS,
    FXID_COUNT,
};

typedef struct SarumanFx
{
    HackLibFxColor color;

    // sound played with it, 0 for none
    u16 sfx;

    // HackLibFxShape, or a HackLibFxAura row when frames is set
    u8 shape;

    // ticks per frame, 0 for the stock pace
    u8 ticks;

    // pixels drawn above the position
    u8 lift;

    // aura duration in frames, 0 for a one-shot shape
    u8 frames;
} SarumanFx;

static const SarumanFx FxTable[FXID_COUNT] = {
    [FXID_FLAME_TRAIL] = {FX_COLOR_EMBER, 0, FX_SHAPE_SMOKE_SMALL, 1, BOLT_LIFT, 0},
    [FXID_FUSE_TRAIL] = {FX_COLOR_GOLD, 0, FX_SHAPE_SPARK, 2, BOLT_LIFT, 0},
    [FXID_WISP_TRAIL] = {FX_COLOR_GREEN, 0, FX_SHAPE_SMOKE_SMALL, 2, BOLT_LIFT, 0},
    [FXID_FLAME_HIT] = {FX_COLOR_EMBER, SFX_COMBAT_FIREBURST_PROC, FX_SHAPE_POOL_FLASH, 2, 0, 0},
    [FXID_FLAME_LICK] = {FX_COLOR_EMBER, 0, FX_SHAPE_TENDRIL_BURST, 3, 0, 0},
    [FXID_BLAST_WAVE] = {FX_COLOR_NIGHT, SFX_COMBAT_TRIPLE_DAMAGE, FX_SHAPE_SHOCKWAVE, 3, 0, 0},
    [FXID_BLAST_SMOKE] = {FX_COLOR_NAZGUL, 0, FX_SHAPE_SMOKE_RISE, 4, SMOKE_LIFT, 0},
    [FXID_BLAST_FLASH] = {FX_COLOR_WHITE, 0, FX_SHAPE_STAR_BLOOM, 2, 8, 0},
    [FXID_WISP_CLOUD] = {FX_COLOR_GREEN, SFX_COMBAT_VENOM_PROC, FX_SHAPE_SMOKE_RISE, 3, SMOKE_LIFT,
                         0},
    [FXID_WISP_POOL] = {FX_COLOR_SHADOW, 0, FX_SHAPE_GROUND_POOL, 3, 0, 0},
    [FXID_RIPPLE] = {FX_COLOR_VENOM, 0, FX_SHAPE_OVAL_RING, 3, 0, 0},
    [FXID_RIPPLE_FAR] = {FX_COLOR_GREEN, 0, FX_SHAPE_OVAL_RING, 4, 0, 0},
    [FXID_DAZE] = {FX_COLOR_VENOM, 0, FX_SHAPE_SPARK, 3, 20, 0},
    [FXID_SEAL] = {FX_COLOR_VENOM, SFX_COMBAT_BLOCK, FX_SHAPE_CHECKER_RING, 3, 0, 0},
    [FXID_REMEDY_MOTE] = {FX_COLOR_GOLD, 0, FX_SHAPE_TENDRIL_BURST, 3, 0, 0},
    [FXID_REMEDY_GLOW] = {FX_COLOR_GOLD, 0, FX_SHAPE_SPARKLE_RING, 3, 0, 0},
    [FXID_FURNACE_GLOW] = {FX_COLOR_EMBER, 0, FX_SHAPE_GROUND_POOL, 2, 0, 0},
    [FXID_FURNACE_ERUPT] = {FX_COLOR_EMBER, SFX_COMBAT_FIREBURST_PROC, FX_AURA_FLAME_CROWN, 3, 0,
                            40},
    [FXID_PRISM_WHITE] = {FX_COLOR_WHITE, 0, FX_AURA_BIG_RING, 3, 0, 24},
    [FXID_PRISM_ISENGARD] = {FX_COLOR_EMBER, 0, FX_AURA_BIG_RING, 3, 0, 24},
    [FXID_PRISM_VOICE] = {FX_COLOR_GREEN, 0, FX_AURA_BIG_RING, 3, 0, 24},
    [FXID_STORM_BOLT] = {FX_COLOR_WHITE, SFX_COMBAT_TRIPLE_DAMAGE, FX_SHAPE_LIGHTNING, 2, 0, 0},
    [FXID_STORM_WAVE] = {FX_COLOR_GHOST, SFX_COMBAT_SUNBURST, FX_SHAPE_SHOCKWAVE, 3, 0, 0},
    [FXID_FIRE_WAVE] = {FX_COLOR_NIGHT, SFX_COMBAT_FIREBURST_PROC, FX_SHAPE_SHOCKWAVE_ALT, 3, 0, 0},
    [FXID_SPITE_BLOOD] = {FX_COLOR_BLOOD, 0, FX_SHAPE_TENDRIL_BURST, 3, 0, 0},
    [FXID_SPITE_DRAIN] = {FX_COLOR_BLOOD, 0, FX_SHAPE_SPARKLE_RING, 3, 0, 0},
    [FXID_POWDER_FLASH] = {FX_COLOR_EMBER, SFX_COMBAT_FIREBURST_PROC, FX_SHAPE_STAR_BLOOM, 3, 0, 0},
    [FXID_POWDER_SMOKE] = {FX_COLOR_NAZGUL, 0, FX_SHAPE_SMOKE_SMALL, 3, 6, 0},
    [FXID_FURNACE_SMOKE] = {FX_COLOR_NAZGUL, 0, FX_SHAPE_SMOKE_RISE, 4, SMOKE_LIFT, 0},
    [FXID_GEM_WHITE] = {FX_COLOR_WHITE, 0, FX_SHAPE_SPARK, 5, GEM_LIFT, 0},
    [FXID_GEM_ISENGARD] = {FX_COLOR_EMBER, 0, FX_SHAPE_SPARK, 5, GEM_LIFT, 0},
    [FXID_GEM_VOICE] = {FX_COLOR_VENOM, 0, FX_SHAPE_SPARK, 5, GEM_LIFT, 0},
    [FXID_DAZE_ORBIT] = {FX_COLOR_GREEN, 0, FX_AURA_ORBIT, 0, 0, 75},
    [FXID_ECHO_RING] = {FX_COLOR_GOLD, 0, FX_SHAPE_WIDE_RING, 2, 0, 0},
    [FXID_ECHO_REPEAT] = {FX_COLOR_WHITE, 0, FX_SHAPE_WIDE_RING, 3, 0, 0},
    [FXID_SPITE_ARCS] = {FX_COLOR_BLOOD, SFX_COMBAT_HIT_IMPALE, FX_SHAPE_SHOCKWAVE_ALT, 3, 0, 0},
};

static Actor *castFx(Actor *on, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(on, shape, color, layer), CAST_FX_TICKS);
}

static Actor *slowFx(Actor *on, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(on, shape, color, layer), SLOW_FX_TICKS);
}

// Many-Coloured Ward bubble colors, cycled while the ward lasts. NAZGUL is the stock blue shield.
static const u8 WardSlots[] = {FX_COLOR_NAZGUL, FX_COLOR_EMBER, FX_COLOR_NIGHT, FX_COLOR_WHITE};
#define WARD_COLOR_FRAMES 20

static void tickWard(Actor *fx)
{
    s16 left = fx->as.combat.venomDuration;

    if (left > 0)
    {
        fx->oam.paletteNumber = WardSlots[left / WARD_COLOR_FRAMES % ARRAY_COUNT(WardSlots)];
    }
    fx_tickTimedAttachment(fx);
}

static void playFx(Actor *origin, s32 x, s32 y, u8 id)
{
    const SarumanFx *fx = &FxTable[id];
    Vector2Fp16 pos = {x << 16, (y - fx->lift) << 16};
    Actor *spawned;

    if (fx->frames != 0)
    {
        spawned = hackLib_fxAura(origin, fx->shape, fx->color, fx->frames, FX_LAYER_BEHIND);
        if (spawned != NULL)
        {
            hackLib_fxDetach(spawned, &pos);
        }
    }
    else
    {
        spawned = hackLib_fxAt(origin, &pos, fx->shape, fx->color);
    }
    if (fx->ticks != 0)
    {
        hackLib_fxPace(spawned, fx->ticks);
    }
    if (fx->sfx != 0)
    {
        sfx_play(fx->sfx);
    }
}

/** Play effect id at at after delay frames, or now when delay is 0 or every cue is taken. */
static void cue(u8 playerIndex, Actor *origin, const Vector2Fp16 *at, u8 delay, u8 id)
{
    SarumanCue *cues = state(playerIndex)->cues;
    s32 x = at->x >> 16;
    s32 y = at->y >> 16;

    if (delay != 0)
    {
        for (u32 i = 0; i < CUE_COUNT; i++)
        {
            if (cues[i].fx == FXID_NONE)
            {
                cues[i] = (SarumanCue){x, y, delay, id};
                return;
            }
        }
    }
    playFx(origin, x, y, id);
}

/** Cue effect id on up to FOE_MARK_MAX foes within radius of center, step frames apart. */
static void markFoes(Actor *origin, Actor *center, u32 radius, u8 id, u8 delay, u8 step)
{
    u8 player = origin->as.combat.playerIndex;
    Actor *foe = NULL;
    u32 marked = 0;

    while (marked < FOE_MARK_MAX &&
           (foe = hackLib_nextTargetAt(&center->coords, NULL, radius, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe))
        {
            cue(player, origin, &foe->coords, delay + step * marked, id);
            marked++;
        }
    }
}

/** Fly a trail of effect trail from from to target. Impacts cued BOLT_FRAMES later land with it. */
static void launchBolt(u8 playerIndex, Actor *from, Actor *target, u8 trail)
{
    state(playerIndex)->bolt = (SarumanBolt){from->coords.x >> 16,
                                             from->coords.y >> 16,
                                             target->coords.x >> 16,
                                             target->coords.y >> 16,
                                             0,
                                             trail};
}

static void tickSequence(u8 playerIndex, Actor *actor)
{
    SarumanState *st = state(playerIndex);
    SarumanBolt *bolt = &st->bolt;

    for (u32 i = 0; i < CUE_COUNT; i++)
    {
        SarumanCue *pending = &st->cues[i];

        if (pending->fx != FXID_NONE && --pending->delay == 0)
        {
            u8 id = pending->fx;

            pending->fx = FXID_NONE;
            playFx(actor, pending->x, pending->y, id);
        }
    }
    if (bolt->trail == FXID_NONE)
    {
        return;
    }
    if (++bolt->frame % BOLT_TRAIL_STEP == 0)
    {
        playFx(actor, bolt->fromX + (bolt->toX - bolt->fromX) * bolt->frame / BOLT_FRAMES,
               bolt->fromY + (bolt->toY - bolt->fromY) * bolt->frame / BOLT_FRAMES, bolt->trail);
    }
    if (bolt->frame >= BOLT_FRAMES)
    {
        bolt->trail = FXID_NONE;
    }
}

static void burnPulse(HackLibTimedEffect *effect)
{
    hackLib_fxPace(
        hackLib_fxBurst(effect->target, FX_SHAPE_SMOKE_SMALL, FX_COLOR_EMBER, FX_LAYER_FRONT), 2);
    skillExpansion_dealDamage(effect->owner, effect->target, effect->magnitude,
                              HACKLIB_DAMAGE_PERIODIC, effect->abilityRow);
}

static const HackLibEffectDef Burn = {
    .id = SARUMAN_BURN_ID,
    .interval = 30,
    .maxStacks = 1,
    .stackRule = HACKLIB_EFFECT_REFRESH,
    .pulse = burnPulse,
};
static const HackLibEffectDef Guard = {
    .id = SARUMAN_GUARD_ID,
    .maxStacks = 1,
    .stackRule = HACKLIB_EFFECT_REPLACE,
};

static bool32 refIs(HackLibActorRef reference, u16 frames, Actor *actor)
{
    return frames != 0 && reference.actor == actor && hackLib_isActorRefValid(reference);
}

static Actor *refActor(HackLibActorRef reference, u16 frames)
{
    if (frames == 0 || !hackLib_isActorRefValid(reference) || reference.actor->currentHp <= 0)
    {
        return NULL;
    }
    return reference.actor;
}

static void grantSpirit(u8 playerIndex, s32 amount)
{
    if (amount > 0)
    {
        player_addSpirit(playerIndex, amount);
    }
}

static const HackLibTimedEffect *findBurn(Actor *owner, Actor *target)
{
    for (u32 i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        const HackLibTimedEffect *effect = hackLib_effectAt(i);
        if (effect && effect->definition == &Burn && effect->owner == owner &&
            effect->target == target)
        {
            return effect;
        }
    }
    return NULL;
}

/** Set target burning. A stronger burn replaces a weaker one, otherwise the longer time wins. */
static void applyBurn(Actor *actor, Actor *target, u32 frames, s32 magnitude, u8 row)
{
    u8 player = actor->as.combat.playerIndex;
    s32 fires = passiveLevel(player, SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY);
    const HackLibTimedEffect *burning = findBurn(actor, target);

    magnitude += FIRES_DAMAGE_PER_LEVEL * fires;
    if (burning != NULL && burning->magnitude < magnitude)
    {
        hackLib_removeEffect((HackLibTimedEffect *)burning);
    }
    hackLib_applyEffect(&Burn, actor, target, frames + FIRES_FRAMES_PER_LEVEL * fires, magnitude,
                        row);
}

/** Stun or hold frames after Honeyed Words. */
static u32 stunFrames(u8 playerIndex, u32 frames)
{
    return scale(frames,
                 100 + HONEYED_PERCENT_PER_LEVEL *
                           passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HONEYED_WORDS));
}

/** Curse target with a wisp flown from from. */
static void curse(u8 playerIndex, Actor *from, Actor *target, u32 level)
{
    SarumanState *st = state(playerIndex);
    Actor *origin = PLAYER(playerIndex).ownerActor;

    st->curse = hackLib_actorRef(target);
    st->curseFrames = 480 + 60 * level;
    st->curseLevel = level;
    launchBolt(playerIndex, from, target, FXID_WISP_TRAIL);
    cue(playerIndex, origin, &target->coords, 0, FXID_WISP_POOL);
    cue(playerIndex, origin, &target->coords, 3, FXID_WISP_CLOUD);
    hackLib_fxAura(target, FX_AURA_SWIRL, FX_COLOR_GREEN, st->curseFrames, FX_LAYER_FRONT);
}

/** Spiteful blast around Saruman, stronger the more HP he is missing. */
static u32 spiteBlast(Actor *actor, u32 level, s32 power)
{
    u8 player = actor->as.combat.playerIndex;
    s32 maxHp = PLAYER_STAT(player, STAT_MAX_HP);
    s32 missing = maxHp > 0 ? 100 - PLAYER(player).currentHp * 100 / maxHp : 0;
    SkillExpansionBurst burst = {.attacker = actor,
                                 .center = actor,
                                 .radius = SPITE_RADIUS,
                                 .kind = HACKLIB_DAMAGE_ABILITY,
                                 .abilityRow = SARUMAN_SPITE};
    u32 hits;

    if (missing < 0)
    {
        missing = 0;
    }
    burst.damage = scale(scale(14 + 5 * level, 100 + missing), power);
    markFoes(actor, actor, SPITE_RADIUS, FXID_SPITE_BLOOD, 0, 0);
    hits = skillExpansion_burstDamage(&burst);
    s32 refund = SPITE_STEAL_SPIRIT * hits;
    grantSpirit(player, refund < SPITE_SPIRIT_CAP ? refund : SPITE_SPIRIT_CAP);
    slowFx(actor, FX_SHAPE_GROUND_POOL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    cue(player, actor, &actor->coords, 0, FXID_SPITE_ARCS);
    if (hits != 0)
    {
        cue(player, actor, &actor->coords, 10, FXID_SPITE_DRAIN);
    }
    return hits;
}

static u32 chargeCount(u8 playerIndex)
{
    return state(playerIndex)->spellCharges;
}

static bool32 canEcho(u8 row)
{
    return row < HERO_ACTIVE_SKILL_COUNT || row == SARUMAN_FURNACE || row == SARUMAN_CURSE ||
           row == SARUMAN_LIGHT_BROKEN || row == SARUMAN_SPITE;
}

static SkillExpansionCastResult castRow(Actor *actor, u8 player, u8 row, u32 level, s32 power)
{
    SarumanState *st = state(player);
    Actor *target;
    SkillExpansionBurst burst = {
        .attacker = actor, .center = actor, .kind = HACKLIB_DAMAGE_ABILITY, .abilityRow = row};

    if (level > ACTIVE_SKILL_MAX_LEVEL + 1)
    {
        level = ACTIVE_SKILL_MAX_LEVEL + 1;
    }
    if (level == 0)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    switch (row)
    {
    case SARUMAN_VOICE:
        burst.radius = 64;
        burst.damage = scale(4 + 3 * level, power);
        burst.stunFrames = stunFrames(player, 45 + 15 * level);
        if (!skillExpansion_burstDamage(&burst))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        cue(player, actor, &actor->coords, 0, FXID_RIPPLE);
        cue(player, actor, &actor->coords, 5, FXID_RIPPLE);
        cue(player, actor, &actor->coords, 10, FXID_RIPPLE_FAR);
        markFoes(actor, actor, burst.radius, FXID_DAZE, 0, 0);
        markFoes(actor, actor, burst.radius, FXID_DAZE_ORBIT, 0, 0);
        sfx_play(SARUMAN_VOICE_SFX);
        return SKILL_CAST_SUCCESS;
    case SARUMAN_BLASTING_FIRE:
        target = skillExpansion_findTarget(actor, 128, NULL, 0);
        if (!target)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        burst.center = target;
        burst.radius = 40;
        burst.damage = scale(20 + 7 * level, power);
        burst.stunFrames = stunFrames(player, 30);
        launchBolt(player, actor, target, FXID_FUSE_TRAIL);
        cue(player, actor, &target->coords, BOLT_FRAMES, FXID_BLAST_FLASH);
        cue(player, actor, &target->coords, BOLT_FRAMES + 1, FXID_BLAST_WAVE);
        cue(player, actor, &target->coords, BOLT_FRAMES + 6, FXID_BLAST_SMOKE);
        sfx_play(SARUMAN_CAST_SFX);
        if (!skillExpansion_burstDamage(&burst))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        return SKILL_CAST_SUCCESS;
    case SARUMAN_WHITE_HAND:
        burst.radius = 104;
        burst.damage = scale(36 + 12 * level, power);
        burst.stunFrames = stunFrames(player, 60);
        markFoes(actor, actor, burst.radius, FXID_STORM_BOLT, 6, 4);
        if (!skillExpansion_burstDamage(&burst))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        hackLib_fxPace(
            hackLib_fxAura(actor, FX_AURA_GROUND_RINGS, FX_COLOR_NIGHT, 60, FX_LAYER_BEHIND),
            CAST_FX_TICKS);
        hackLib_fxBurst(actor, FX_SHAPE_SKY_RAIN, FX_COLOR_WHITE, FX_LAYER_FRONT);
        slowFx(actor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_NIGHT, FX_LAYER_FRONT);
        cue(player, actor, &actor->coords, 24, FXID_STORM_WAVE);
        sfx_play(SARUMAN_CAST_SFX);
        return SKILL_CAST_SUCCESS;
    case SARUMAN_FLAME: {
        target = skillExpansion_findTarget(actor, 128, NULL, 0);
        if (!target)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        HackLibActorRef victim = hackLib_actorRef(target);
        Vector2Fp16 impact = target->coords;
        castFx(actor, FX_SHAPE_POOL_FLASH, FX_COLOR_EMBER, FX_LAYER_FRONT);
        launchBolt(player, actor, target, FXID_FLAME_TRAIL);
        cue(player, actor, &impact, BOLT_FRAMES, FXID_FLAME_HIT);
        cue(player, actor, &impact, BOLT_FRAMES + 3, FXID_FLAME_LICK);
        HackLibDamageResult result = skillExpansion_dealDamage(
            actor, target, scale(10 + 5 * level, power), HACKLIB_DAMAGE_ABILITY, row);
        if (result.hit && !result.killed && hackLib_isActorRefValid(victim) &&
            target->currentHp > 0)
        {
            applyBurn(actor, target, 120 + 30 * level, scale(1 + level, power), row);
        }
        sfx_play(SARUMAN_CAST_SFX);
        return SKILL_CAST_SUCCESS;
    }
    case SARUMAN_GUARD:
        if (!hackLib_applyEffect(&Guard, actor, actor, 180 + 60 * level, 20 + 5 * level, row))
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        castFx(actor, FX_SHAPE_RING_SPARKLES, FX_COLOR_WHITE, FX_LAYER_FRONT);
        target = hackLib_fxAura(actor, FX_AURA_SHIELD, FX_COLOR_NAZGUL, 180 + 60 * level,
                                FX_LAYER_FRONT);
        if (target != NULL)
        {
            target->tickCallback = tickWard;
        }
        sfx_play(SFX_COMBAT_GANDALF_SHIELD_CAST);
        return SKILL_CAST_SUCCESS;
    case SARUMAN_BINDING: {
        target = skillExpansion_findTarget(actor, 128, NULL, 0);
        if (!target)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        HackLibActorRef victim = hackLib_actorRef(target);
        HackLibDamageResult result = skillExpansion_dealDamage(
            actor, target, scale(6 + 3 * level, power), HACKLIB_DAMAGE_ABILITY, row);
        if (result.hit && !result.killed && hackLib_isActorRefValid(victim) &&
            target->currentHp > 0)
        {
            st->bound = victim;
            st->boundFrames = stunFrames(player, 120 + 30 * level);
            hackLib_applyStun(target, st->boundFrames);
            cue(player, actor, &target->coords, 0, FXID_SEAL);
            hackLib_fxPace(hackLib_fxAura(target, FX_AURA_GROUND_RINGS, FX_COLOR_GREEN,
                                          st->boundFrames, FX_LAYER_BEHIND),
                           SLOW_FX_TICKS);
        }
        sfx_play(SARUMAN_VOICE_SFX);
        return SKILL_CAST_SUCCESS;
    }
    case SARUMAN_HEAL: {
        s32 keeper = passiveLevel(player, SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC);
        s32 missing = PLAYER_STAT(player, STAT_MAX_HP) - PLAYER(player).currentHp;
        s32 heal = scale(scale(12 + 8 * level, power), 100 + KEEPER_PERCENT_PER_LEVEL * keeper);

        if (missing <= 0)
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        skillExpansion_addHp(player, heal, FALSE);
        if (keeper > 0)
        {
            s32 refund = heal - missing;
            grantSpirit(player, refund < REMEDY_SPIRIT_CAP ? refund : REMEDY_SPIRIT_CAP);
        }
        hackLib_fxPace(
            hackLib_fxAura(actor, FX_AURA_GROUND_RINGS, FX_COLOR_GOLD, 45, FX_LAYER_BEHIND),
            CAST_FX_TICKS);
        slowFx(actor, FX_SHAPE_TENDRIL_BURST, FX_COLOR_WHITE, FX_LAYER_FRONT);
        cue(player, actor, &actor->coords, 4, FXID_REMEDY_MOTE);
        cue(player, actor, &actor->coords, 12, FXID_REMEDY_GLOW);
        sfx_play(SFX_HERB_PICKUP);
        return SKILL_CAST_SUCCESS;
    }
    case SARUMAN_ECHO:
        if (st->lastRow == ROW_NONE || !canEcho(st->lastRow))
        {
            return SKILL_CAST_UNAVAILABLE;
        }
        cue(player, actor, &actor->coords, 0, FXID_ECHO_RING);
        cue(player, actor, &actor->coords, 8, FXID_ECHO_REPEAT);
        return castRow(actor, player, st->lastRow,
                       skillExpansion_effectiveActiveLevel(player, st->lastRow),
                       scale(40 + 10 * level, power));
    case SARUMAN_FURNACE: {
        u32 affected = 0;
        target = NULL;
        while (affected < FURNACE_TARGET_MAX &&
               (target = hackLib_nextTargetAt(&actor->coords, actor, 72, target)) != NULL)
        {
            if (skillExpansion_isFoe(target))
            {
                applyBurn(actor, target, 180, scale(2 + 2 * level, power), row);
                if (affected < FOE_MARK_MAX)
                {
                    cue(player, actor, &target->coords, 0, FXID_FURNACE_GLOW);
                    cue(player, actor, &target->coords, 4 + 2 * affected, FXID_FURNACE_ERUPT);
                }
                affected++;
            }
        }
        if (affected == 0)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        hackLib_fxAura(actor, FX_AURA_RISING_SPARKS, FX_COLOR_EMBER, 52, FX_LAYER_FRONT);
        cue(player, actor, &actor->coords, 0, FXID_FIRE_WAVE);
        cue(player, actor, &actor->coords, 8, FXID_FURNACE_SMOKE);
        sfx_play(SARUMAN_CAST_SFX);
        return SKILL_CAST_SUCCESS;
    }
    case SARUMAN_CURSE:
        target = skillExpansion_findTarget(actor, 128, NULL, 0);
        if (!target)
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        curse(player, actor, target, level);
        sfx_play(SARUMAN_VOICE_SFX);
        skillExpansion_dealDamage(actor, target, scale(5 + 3 * level, power),
                                  HACKLIB_DAMAGE_ABILITY, row);
        return SKILL_CAST_SUCCESS;
    case SARUMAN_LIGHT_BROKEN:
        burst.radius = 80;
        burst.damage = scale(28 + 10 * level, power);
        if (!skillExpansion_burstDamage(&burst))
        {
            return SKILL_CAST_INVALID_TARGET;
        }
        castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        cue(player, actor, &actor->coords, 2, FXID_PRISM_WHITE);
        cue(player, actor, &actor->coords, 8, FXID_PRISM_ISENGARD);
        cue(player, actor, &actor->coords, 14, FXID_PRISM_VOICE);
        markFoes(actor, actor, burst.radius, FXID_GEM_WHITE, 16, 2);
        sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
        return SKILL_CAST_SUCCESS;
    case SARUMAN_SPITE:
        return spiteBlast(actor, level, power) ? SKILL_CAST_SUCCESS : SKILL_CAST_INVALID_TARGET;
    default:
        return SKILL_CAST_UNAVAILABLE;
    }
}

static bool isDamagingSpell(u8 row)
{
    return row != SARUMAN_GUARD && row != SARUMAN_HEAL;
}

static SkillExpansionCastResult resolve(Actor *actor)
{
    u8 player = actor->as.combat.playerIndex;
    u8 row = PLAYER(player).castingActiveSkill;
    SarumanState *st = state(player);
    SkillExpansionCastResult result;

    if (row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return SKILL_CAST_UNAVAILABLE;
    }
    u8 damageRow = row == SARUMAN_ECHO ? st->lastRow : row;
    bool empowered = st->spellCharges == EMPOWER_CAST_COUNT && isDamagingSpell(damageRow);
    s32 power = empowered ? 100 + EMPOWER_DAMAGE_PERCENT +
                                EMPOWER_POWER_PER_LEVEL *
                                    passiveLevel(player, SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS)
                          : 100;
    result = castRow(actor, player, row, skillExpansion_effectiveActiveLevel(player, row), power);
    if (result == SKILL_CAST_SUCCESS && isSaruman(player))
    {
        if (canEcho(row))
        {
            st->lastRow = row;
        }
        if (empowered)
        {
            st->spellCharges = 0;
            grantSpirit(player,
                        RING_LORE_SPIRIT * passiveLevel(player, SKILL_EXPANSION_PASSIVE_RING_LORE) +
                            (passiveLevel(player, SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL)
                                 ? COUNCIL_SPIRIT
                                 : 0));
            skillExpansion_feedback(player, "Empowered spell!");
            castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
        else if (st->spellCharges < EMPOWER_CAST_COUNT && ++st->spellCharges == EMPOWER_CAST_COUNT)
        {
            skillExpansion_feedback(player, "Empowered!");
            castFx(actor, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
    }
    return result;
}

static s32 incomingDamage(u8 player, Actor *attacker, s32 damage)
{
    s32 robes = ROBES_PERCENT_PER_CHARGE * chargeCount(player) *
                passiveLevel(player, SKILL_EXPANSION_PASSIVE_ROBES_OF_MANY_COLOURS);

    (void)attacker;
    for (u32 i = 0; i < HACKLIB_EFFECT_LIMIT; i++)
    {
        const HackLibTimedEffect *effect = hackLib_effectAt(i);
        if (effect && effect->definition == &Guard && effect->target == PLAYER(player).ownerActor)
        {
            damage -= damage * effect->magnitude / 100;
            break;
        }
    }
    return damage - damage * robes / 100;
}

static const SkillExpansionActiveDef *const NewActives[SKILL_EXPANSION_NEW_ACTIVE_COUNT];
static const ActiveSkill Actives[HERO_ACTIVE_SKILL_COUNT];

/** Restore each row's normal Spirit cost. */
static void applyCosts(u8 playerIndex)
{
    for (u32 row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
    {
        const ActiveSkill *source = NULL;
        u32 cost;

        if (row < HERO_ACTIVE_SKILL_COUNT)
        {
            source = &Actives[row];
        }
        else if (row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW &&
                 NewActives[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW] != NULL)
        {
            source = NewActives[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW]->row;
        }
        if (source == NULL)
        {
            continue;
        }
        cost = source->spiritCost;
        skillExpansion_players[playerIndex].activeSkills[row].spiritCost = cost;
    }
}

static void clearState(u8 playerIndex)
{
    memory_memset(state(playerIndex), 0, sizeof(SarumanState));
    state(playerIndex)->lastRow = ROW_NONE;
    applyCosts(playerIndex);
}

static void reset(u8 playerIndex)
{
    if (isSaruman(playerIndex))
    {
        clearState(playerIndex);
    }
}

static void cleanup(u8 playerIndex, Actor *actor)
{
    (void)actor;
    reset(playerIndex);
}

/** Wildfire: set a foe near where a burning foe died alight, preferring one not yet burning. */
static void leapWildfire(u8 playerIndex, Actor *actor)
{
    SarumanState *st = state(playerIndex);
    s32 wildfire = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WILDFIRE);
    u32 flame = skillExpansion_effectiveActiveLevel(playerIndex, SARUMAN_FLAME);
    Vector2Fp16 at = {(s32)st->wildfireX * 0x10000, (s32)st->wildfireY * 0x10000};
    Actor *target = NULL;
    Actor *pick = NULL;

    st->wildfirePending = FALSE;
    while ((target = hackLib_nextTargetAt(&at, actor, WILDFIRE_RADIUS, target)) != NULL)
    {
        if (!skillExpansion_isFoe(target) || target->currentHp <= 0)
        {
            continue;
        }
        if (pick == NULL)
        {
            pick = target;
        }
        if (findBurn(actor, target) == NULL)
        {
            pick = target;
            break;
        }
    }
    if (pick == NULL || wildfire == 0)
    {
        return;
    }
    applyBurn(actor, pick, WILDFIRE_FRAMES + WILDFIRE_FRAMES_PER_LEVEL * wildfire,
              2 + (flame != 0 ? flame : 1), SARUMAN_FLAME);
    cue(playerIndex, actor, &pick->coords, 0, FXID_FLAME_HIT);
    cue(playerIndex, actor, &pick->coords, 3, FXID_FLAME_LICK);
}

static void tick(u8 playerIndex, Actor *actor)
{
    SarumanState *st;
    s32 maxHp;
    bool32 low;
    Actor *cursed;

    if (!isSaruman(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    if (PLAYER(playerIndex).currentHp <= 0)
    {
        clearState(playerIndex);
        return;
    }
    tickSequence(playerIndex, actor);
    if (st->wildfirePending)
    {
        leapWildfire(playerIndex, actor);
    }
    if (++st->regenFrames >= REGEN_FRAMES)
    {
        st->regenFrames = 0;
        if (PLAYER(playerIndex).currentSpirit < PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT))
        {
            player_addSpirit(playerIndex, 1);
        }
    }
    if (st->boundFrames != 0)
    {
        st->boundFrames--;
    }
    cursed = refActor(st->curse, st->curseFrames);
    if (cursed == NULL)
    {
        st->curseFrames = 0;
    }
    else if (--st->curseFrames % CURSE_PULSE_FRAMES == 0)
    {
        hackLib_fxPace(
            hackLib_fxBurst(cursed, FX_SHAPE_SMOKE_SMALL, FX_COLOR_GREEN, FX_LAYER_FRONT), 2);
    }
    if (st->partingCooldown != 0)
    {
        st->partingCooldown--;
    }
    maxHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    low = PLAYER(playerIndex).currentHp * 100 < PARTING_HP_PERCENT * maxHp;
    if (low && !st->wasLow && st->partingCooldown == 0 &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_PARTING_SHOT) > 0)
    {
        u32 level = *skillExpansion_activeLevel(playerIndex, SARUMAN_SPITE);
        Actor *target;

        st->partingCooldown = PARTING_COOLDOWN_FRAMES;
        spiteBlast(actor, level != 0 ? level : 1, 100);
        target = skillExpansion_findTarget(actor, 128, NULL, 0);
        if (target != NULL)
        {
            curse(playerIndex, actor, target, level != 0 ? level : 1);
        }
        skillExpansion_feedback(playerIndex, "Parting Shot");
    }
    st->wasLow = low;
    applyCosts(playerIndex);
}

static bool32 isStunned(Actor *actor)
{
    return actor != NULL && actor->currentHp > 0 && actor->actionState == ACTOR_STATE_STUNNED;
}

static s32 meleeDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    (void)playerIndex;
    (void)victim;
    return damage;
}

static bool32 isSpellDamage(void)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();

    return source != NULL && source->kind != HACKLIB_DAMAGE_WEAPON &&
           source->kind != HACKLIB_DAMAGE_PROJECTILE && source->kind != HACKLIB_DAMAGE_UNKNOWN;
}

/** Spell damage other than a burn or other damage over time. */
static bool32 isSpellHit(void)
{
    return isSpellDamage() && hackLib_currentDamageSource()->kind != HACKLIB_DAMAGE_PERIODIC;
}

static s32 rangedDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    SarumanState *st;
    s32 percent = 100;
    s32 maxHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);

    if (!isSaruman(playerIndex) || !isSpellDamage())
    {
        return damage;
    }
    st = state(playerIndex);
    if (refIs(st->curse, st->curseFrames, victim))
    {
        percent += CURSE_PERCENT + CURSE_PERCENT_PER_LEVEL * st->curseLevel;
    }
    if (refIs(st->bound, st->boundFrames, victim))
    {
        percent += BIND_PERCENT;
    }
    if (isSpellHit() && findBurn(PLAYER(playerIndex).ownerActor, victim) != NULL)
    {
        percent += KINDLING_PERCENT_PER_LEVEL *
                   passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_KINDLING);
    }
    if (isSpellHit() && isStunned(victim))
    {
        percent += COMMAND_PERCENT_PER_LEVEL *
                   passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_VOICE_OF_COMMAND);
    }
    if (PLAYER(playerIndex).currentHp * 100 < SPITE_HP_PERCENT * maxHp)
    {
        percent +=
            SPITE_PERCENT_PER_LEVEL * passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SPITE);
    }
    return scale(damage, percent);
}

static void onHit(u8 playerIndex, Actor *attacker, Actor *victim, HackLibDamageResult result,
                  bool melee)
{
    s32 staff = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STAFF_OF_ORTHANC);

    if (!isSaruman(playerIndex) || !result.hit)
    {
        return;
    }
    SarumanState *st = state(playerIndex);
    if (melee && result.damageDealt > 0 && skillExpansion_isFoe(victim))
    {
        grantSpirit(playerIndex,
                    STAFF_HIT_SPIRIT +
                        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WISDOM_OF_THE_ISTARI) +
                        (isStunned(victim)
                             ? passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_TREACHERY)
                             : 0));
    }
    if (result.damageDealt > 0 && (melee || isSpellHit()) &&
        refIs(st->curse, st->curseFrames, victim))
    {
        grantSpirit(playerIndex, CURSE_LEAK_SPIRIT);
        hackLib_fxBurst(victim, FX_SHAPE_SPARK, FX_COLOR_VENOM, FX_LAYER_FRONT);
    }
    if (melee && staff > 0 && !result.killed && skillExpansion_isFoe(victim))
    {
        applyBurn(attacker, victim, STAFF_BURN_FRAMES, 1 + staff, ROW_NONE);
    }
}

static void onKill(u8 playerIndex, Actor *attacker, Actor *victim)
{
    const HackLibDamageSource *source = hackLib_currentDamageSource();
    bool32 spell = isSpellDamage();

    if (!isSaruman(playerIndex))
    {
        return;
    }
    SarumanState *st = state(playerIndex);
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WILDFIRE) > 0 &&
        findBurn(PLAYER(playerIndex).ownerActor, victim) != NULL)
    {
        // Leaps next tick, since a burn's own pulse may be dealing this kill.
        st->wildfireX = victim->coords.x >> 16;
        st->wildfireY = victim->coords.y >> 16;
        st->wildfirePending = TRUE;
    }
    if (spell)
    {
        grantSpirit(playerIndex,
                    MACHINE_SPIRIT *
                        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR));
    }
    if (spell && source->kind != HACKLIB_DAMAGE_SECONDARY &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC) > 0)
    {
        SkillExpansionBurst burst = {.attacker = attacker,
                                     .center = victim,
                                     .exclude = victim,
                                     .radius = POWDER_RADIUS,
                                     .damage = POWDER_DAMAGE + PLAYER(playerIndex).level,
                                     .kind = HACKLIB_DAMAGE_SECONDARY,
                                     .abilityRow = ROW_NONE};

        cue(playerIndex, attacker, &victim->coords, 0, FXID_POWDER_FLASH);
        cue(playerIndex, attacker, &victim->coords, 4, FXID_POWDER_SMOKE);
        skillExpansion_burstDamage(&burst);
    }
    if (st->curseFrames != 0 && st->curse.actor == victim && st->curse.generation != 0 &&
        st->curse.generation == hackLib_actorRef(victim).generation)
    {
        s32 cutpurse = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_CUTPURSE);
        Actor *next =
            cutpurse > 0 ? skillExpansion_findTarget(victim, CUTPURSE_RANGE, NULL, 0) : NULL;

        st->curseFrames = 0;
        grantSpirit(playerIndex, CUTPURSE_SPIRIT * cutpurse);
        if (next != NULL)
        {
            curse(playerIndex, victim, next, st->curseLevel);
        }
    }
}

static const HackLibHudMeterLook EmpoweredLooks[2] = {
    {HACKLIB_HUD_COLOR_PALE_GOLD, HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_DARK_GOLD,
     HACKLIB_HUD_COLOR_WHITE},
    {HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_DARK_GOLD, HACKLIB_HUD_COLOR_BROWN,
     HACKLIB_HUD_COLOR_WHITE},
};

/** One meter fills across three casts and flashes gold while empowered. */
static void drawEmpowerment(u8 playerIndex)
{
    if (!isSaruman(playerIndex))
    {
        return;
    }
    const HackLibHudMeterLook *look = &EmpoweredLooks[0];
    if (chargeCount(playerIndex) == EMPOWER_CAST_COUNT)
    {
        look = &EmpoweredLooks[(g_EngineState->vblankCounter >> HUD_PULSE_SHIFT) & 1];
    }
    hackLib_drawHudMeter(&HackLibHudGapMeter, chargeCount(playerIndex), EMPOWER_CAST_COUNT, look,
                         NULL, 0);
}

static bool32 canCastEcho(u8 playerIndex)
{
    return state(playerIndex)->lastRow != ROW_NONE;
}

#define CAST_ROW(cost)                                                                             \
    {.spiritCost = cost,                                                                           \
     .field_0x20 = 1,                                                                              \
     .castFlags = {.d = {.forcePose0 = 1}},                                                        \
     .field_0x22 = 5,                                                                              \
     .castTriggerFrame = 0,                                                                        \
     .field_0x24 = 0xff}
#define POLICY(frames) {1, SKILL_EXPANSION_RULE_NONE, SKILL_EXPANSION_RULE_NONE, frames}
static const SkillExpansionCastPolicy QuickPolicy = POLICY(12);
static const SkillExpansionCastPolicy NormalPolicy = POLICY(30);
static const SkillExpansionCastPolicy LongPolicy = POLICY(90);
static const SkillExpansionCastPolicy UltimatePolicy = POLICY(180);

static const ActiveSkill Actives[HERO_ACTIVE_SKILL_COUNT] = {
    CAST_ROW(18), CAST_ROW(18), CAST_ROW(36), CAST_ROW(30), CAST_ROW(24), CAST_ROW(32),
};
static const void *const Icons[HERO_ACTIVE_SKILL_COUNT] = {
    SarumanIconVoiceOfSaruman,   SarumanIconFlameOfOrthanc, SarumanIconBlastingFire,
    SarumanIconManyColouredWard, SarumanIconWordOfBinding,  SarumanIconOrthancsRemedy,
};
static const s8 Levels[HERO_ACTIVE_SKILL_COUNT][SKILL_EXPANSION_REQUIRED_LEVEL_COUNT] = {
    {1, 4, 7, 10, 13, -1},  {1, 5, 8, 11, 14, -1},   {4, 7, 10, 13, 16, -1},
    {6, 9, 12, 15, 18, -1}, {8, 11, 14, 17, 20, -1}, {1, 4, 7, 10, 13, -1},
};
static const u16 ActiveIds[HERO_ACTIVE_SKILL_COUNT] = {0xa01, 0xa02, 0xa03, 0xa04, 0xa05, 0xa06};
static const char *const Descriptions[HERO_ACTIVE_SKILL_COUNT] = {
    "Voice of Saruman (Level @1) - Stun foes around you for 1.5s +0.5s/level and deal 4 "
    "+3/level damage. @2 Spirit.",
    "Flame of Orthanc (Level @1) - Scorch the nearest foe for 10 +5/level, then burn it for 1 "
    "+1/level each second. @2 Spirit.",
    "Blasting Fire (Level @1) - Blast the nearest foe and those near it for 20 +7/level damage "
    "and a short stun. @2 Spirit.",
    "Many-Coloured Ward (Level @1) - Protect yourself: take 20% +5%/level less damage for 6s "
    "+2s/level. @2 "
    "Spirit.",
    "Word of Binding (Level @1) - Hold the nearest foe for 4s +1s/level. Your spells deal it +25% "
    "meanwhile. @2 Spirit.",
    "Orthanc's Remedy (Level @1) - Heal yourself for 12 +8/level. Cast 3 spells to empower "
    "your next damaging spell by 50%. @2 Spirit.",
};
// Columns White | Isengard | Voice once the tree sorts them by first level.
static const u8 Passives[HERO_PASSIVE_SKILL_COUNT] = {
    SKILL_EXPANSION_PASSIVE_ROBES_OF_MANY_COLOURS,
    SKILL_EXPANSION_PASSIVE_KINDLING,
    SKILL_EXPANSION_PASSIVE_HONEYED_WORDS,
    SKILL_EXPANSION_PASSIVE_KEEPER_OF_ORTHANC,
    SKILL_EXPANSION_PASSIVE_WILDFIRE,
    SKILL_EXPANSION_PASSIVE_VOICE_OF_COMMAND,
    SKILL_EXPANSION_PASSIVE_WISDOM_OF_THE_ISTARI,
    SKILL_EXPANSION_PASSIVE_STAFF_OF_ORTHANC,
    SKILL_EXPANSION_PASSIVE_TREACHERY,
};
static const s8 EchoLevels[] = {10, 13, 16, 19, 22, -1};
static const s8 FurnaceLevels[] = {11, 14, 17, 20, 23, -1};
static const s8 CurseLevels[] = {12, 15, 18, 21, 24, -1};
static const s8 LightBrokenLevels[] = {13, 16, 19, 22, 25, -1};
static const s8 SpiteLevels[] = {14, 17, 20, 23, 25, -1};
static const s8 WhiteHandLevels[] = {15, 18, 21, 24, 25, -1};
static const ActiveSkill EchoRow = CAST_ROW(32);
static const ActiveSkill FurnaceRow = CAST_ROW(55);
static const ActiveSkill CurseRow = CAST_ROW(30);
static const ActiveSkill LightBrokenRow = CAST_ROW(55);
static const ActiveSkill SpiteRow = CAST_ROW(45);
static const ActiveSkill WhiteHandRow = CAST_ROW(90);
#define ELITE(name, rowData, iconData, levels, identity, text, rule, condition)                    \
    static const SkillExpansionActiveDef name = {.row = &rowData,                                  \
                                                 .icon = iconData,                                 \
                                                 .requiredLevels = levels,                         \
                                                 .description = text,                              \
                                                 .canCast = condition,                             \
                                                 .resolve = resolve,                               \
                                                 .baseRow = 0,                                     \
                                                 .policy = &rule,                                  \
                                                 .id = identity}
ELITE(Echo, EchoRow, SarumanIconEchoOfPower, EchoLevels, 0xa10,
      "Echo of Power (Level @1) - Repeat your last non-ultimate spell at 40% +10%/level "
      "power. @2 Spirit.",
      QuickPolicy, canCastEcho);
ELITE(Furnace, FurnaceRow, SarumanIconFurnacesOfIsengard, FurnaceLevels, 0xa09,
      "Furnaces of Isengard (Level @1) - White Hand. Isengard. Up to 5 nearby foes burn for 2 "
      "+2/level each second for 6s. @2 Spirit.",
      LongPolicy, NULL);
ELITE(Curse, CurseRow, SarumanIconSharkeysCurse, CurseLevels, 0xa11,
      "Sharkey's Curse (Level @1) - The nearest foe takes +20% +5%/level from your spells "
      "and restores 1 Spirit per direct hit. @2 Spirit.",
      NormalPolicy, NULL);
ELITE(LightBroken, LightBrokenRow, SarumanIconLightBroken, LightBrokenLevels, 0xa12,
      "Light Broken (Level @1) - Blast nearby foes for 28 +10/level damage. @2 Spirit.",
      NormalPolicy, NULL);
ELITE(Spite, SpiteRow, SarumanIconSharkeysSpite, SpiteLevels, 0xa13,
      "Sharkey's Spite (Level @1) - Sharkey. Voice. Blast nearby foes for 14 +5/level, +1% per "
      "1% HP you lack. Steal 2 Spirit each, up to 8. @2 Spirit.",
      NormalPolicy, NULL);
ELITE(WhiteHand, WhiteHandRow, SarumanIconPowerOfTheWhiteHand, WhiteHandLevels, 0xa0c,
      "Power of the White Hand (Level @1) - White Hand. Blast a wide area for 36 +12/level and "
      "stun. @2 Spirit.",
      UltimatePolicy, NULL);

static const SkillExpansionActiveDef *const NewActives[SKILL_EXPANSION_NEW_ACTIVE_COUNT] = {
    &Echo, &Furnace, &Curse, &LightBroken, &Spite, &WhiteHand,
};

const SkillExpansionHeroKit SarumanHeroKit = {
    .activeSkills = Actives,
    .icons = Icons,
    .requiredLevels = Levels,
    .descriptions = Descriptions,
    .passiveSkillIds = Passives,
    .artHeroId = HERO_ID_GANDALF,
    .reset = reset,
    .cleanup = cleanup,
    .tick = tick,
    .meleeDamage = meleeDamage,
    .rangedDamage = rangedDamage,
    .onHit = onHit,
    .onKill = onKill,
    .resolvers = {resolve, resolve, resolve, resolve, resolve, resolve},
    .policies = {&NormalPolicy, &QuickPolicy, &NormalPolicy, &LongPolicy, &NormalPolicy,
                 &QuickPolicy},
    .activeIds = ActiveIds,
    .incomingDamage = incomingDamage,
    .drawHud = drawEmpowerment,
    .newActives = {&Echo, &Furnace, &Curse, &LightBroken, &Spite, &WhiteHand},
    .newPassiveSkillIds = {SKILL_EXPANSION_PASSIVE_LORE_OF_COLOURS,
                           SKILL_EXPANSION_PASSIVE_FIRES_OF_INDUSTRY,
                           SKILL_EXPANSION_PASSIVE_CUTPURSE, SKILL_EXPANSION_PASSIVE_RING_LORE,
                           SKILL_EXPANSION_PASSIVE_MACHINE_OF_WAR, SKILL_EXPANSION_PASSIVE_SPITE,
                           SKILL_EXPANSION_PASSIVE_HEAD_OF_THE_COUNCIL,
                           SKILL_EXPANSION_PASSIVE_POWDER_OF_ORTHANC,
                           SKILL_EXPANSION_PASSIVE_PARTING_SHOT},
};
#endif
