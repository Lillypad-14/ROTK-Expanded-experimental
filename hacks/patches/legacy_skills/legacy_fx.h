#pragma once

// Building blocks for porting Two Towers actives: the effect helpers every port uses and templates
// for the common kinds (projectile, burst, heal). A port of one of those kinds is a const def and
// a one-line cast; a port of another kind builds on the helpers. One file per hero
// (legacy_<hero>.c) defines that hero's LegacyActive table.

#include "actor.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "skill.h"
#include "spriteAnimation.h"
#include "types.h"

// A legacy active: its row's definition (the stock row it takes its cast pose and table row from)
// and its Spirit cost in The Two Towers. It casts `cast`, a port of the Two Towers skill, or else
// the definition's stock callback, a ROTK skill standing in.
typedef struct LegacyActive
{
    const SkillExpansionActiveDef *def;
    u16 spiritCost;
    ActiveSkillCallback cast;
    // Spirit the cost adds per level
    u8 spiritPerLevel;
} LegacyActive;

// Each hero's actives, in the skill record's order (after the passives). A NULL def: none yet.
extern const LegacyActive LegacyGandalfActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT];
extern const LegacyActive LegacyFrodoActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT];
extern const LegacyActive LegacyLegolasActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT];
extern const LegacyActive LegacyAragornActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT];
extern const LegacyActive LegacyEowynActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT];
extern const LegacyActive LegacyGimliActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT];

// A number that grows with the skill's level: base + perLevel * level.
typedef struct LegacyScale
{
    s16 base;
    s16 perLevel;
} LegacyScale;

static inline s32 legacyScale(LegacyScale scale, u8 level)
{
    return scale.base + scale.perLevel * level;
}

// What a legacy effect keeps in its combat head.
typedef struct LegacyFxState
{
    union {
        // the foe it acts on
        Actor *target;
        // its template def
        const void *def;
    };
    // damage, or stun ticks
    u16 amount;
    // per-effect phase
    u8 phase;
    // per-effect counter
    u8 count;
} LegacyFxState;

#define LEGACY_FX(fx) ((LegacyFxState *)&(fx)->as.combat.head)

// the flags of an actor a player's attack can hit
#define LEGACY_FOE_HITTABLE_FLAGS 0x10040400
// Lightstrike's cast sound, the stand-in until the Two Towers sounds are ported
#define LEGACY_SOUND_BOLT 0x5a
// legacyFx_playSound plays nothing
#define LEGACY_SOUND_NONE 0

// The level a legacy skill casts at: its Two Towers level, which goes past the stock curves' five.
u8 legacyFx_castingLevel(Actor *playerActor);
// A Two Towers animation the PC host converted, by its Two Towers address, or NULL.
const SpriteAnimation *legacyFx_sprite(u32 address);
// Play a Two Towers animation on an effect (none when 0).
void legacyFx_setSprite(Actor *fx, u32 animation);
// Once a frame: effects that borrowed the weapons' palette slot try for their own again.
void legacyFx_frameTick(void);
// An effect from the caster, standing still, playing `animation`, with no callbacks and its
// LegacyFxState cleared, or NULL.
Actor *legacyFx_spawn(Actor *playerActor, u32 animation);
// An effect that plays `animation` on the caster once, then is gone.
void legacyFx_spawnOnCaster(Actor *playerActor, u32 animation);
// An actor the caster's effects hit: an enemy, or in a duel the opponent of the player behind
// `source` (an effect or the caster, NULL for the current damage source).
bool32 legacyFx_isFoeOf(Actor *source, Actor *other);
bool32 legacyFx_isFoe(Actor *other);
bool32 legacyFx_animationEnded(Actor *fx);
// Mirror the effect's sprite for the western facings, as The Two Towers does: its art faces east.
void legacyFx_faceSprite(Actor *fx);
// TRUE while the effect is within `margin` pixels of the caster's screen.
bool32 legacyFx_onScreen(Actor *fx, s32 margin);
// Put a fresh effect at `pos`: the spawner's position seed is stale, and the effect would jump there.
void legacyFx_place(Actor *fx, Vector2Fp16 pos);
// Remove the effect at the end of this tick.
void legacyFx_end(Actor *fx);
// The effect's last tick step: ends it with a downed caster, then runs the effect actor's tick.
void legacyFx_finishTick(Actor *fx);
void legacyFx_playSound(u16 sound);

// The Two Towers trail lights (emitter group 2, variants 2 and 3)
#define LEGACY_TRAIL_ORANGE 0x0832EB6Cu
#define LEGACY_TRAIL_YELLOW 0x0832EB74u

// A projectile: flies from the caster for `lifetime` ticks. With no blast it hits the first foe
// for its damage and plays `impact` on it (or is gone, with none). With a blast it bursts on the
// first foe, and every foe in the blast takes its damage on the frame it bursts.
typedef struct LegacyProjectileDef
{
    // the projectile by facing
    const u32 *sprites;
    // played on the foe it hits, when it has no blast
    u32 impact;
    // its burst, 0 for none
    u32 blast;
    s32 speed;
    u8 lifetime;
    LegacyScale damage;
    u16 sound;
    u16 blastSound;
    // trail light: sparkles it drops where it flies, sprayed while it bursts (0: none)
    u32 trail;
} LegacyProjectileDef;

void legacyFx_castProjectile(Actor *playerActor, const LegacyProjectileDef *def);
// The caster's projectile from elsewhere (a trap): from `from` along `facing`, for `damage`.
void legacyFx_castProjectileAt(Actor *playerActor, const LegacyProjectileDef *def, Vector2Fp16 from,
                               Direction8 facing, u16 damage);

// A thrown weapon or arrow: flies from the caster for `lifetime` ticks and hits every foe it passes
// with the caster's own attack (`combat_resolvePveAttack`), `bonus` added to the caster's
// `bonusStat` for the hit. A foe already hurt is passed over. It stops at the first foe that does
// not flinch, or at the first foe with LEGACY_THROWN_STOP_ON_HIT.
typedef struct LegacyThrownDef
{
    // the weapon by facing
    const u32 *sprites;
    s32 speed;
    u8 lifetime;
    LegacyScale bonus;
    // the stat the bonus goes to, STAT_DAMAGE_FLAT when 0
    u8 bonusStat;
    // LEGACY_THROWN_* options
    u8 flags;
    u16 sound;
    // after the hit on a foe, or NULL
    void (*onHit)(Actor *fx, Actor *foe);
    // trail lights: sparkles it drops where it flies, the two in turn each tick (0: none)
    u32 trail[2];
} LegacyThrownDef;


#define LEGACY_THROWN_STOP_ON_HIT (1 << 0)
// it starts one step out from the caster's hand
#define LEGACY_THROWN_START_AHEAD (1 << 1)

// The thrown weapon, or NULL when none could spawn.
Actor *legacyFx_castThrown(Actor *playerActor, const LegacyThrownDef *def);
// TRUE while `fx` is a thrown `def` still in flight.
bool32 legacyFx_thrownAlive(Actor *fx, const LegacyThrownDef *def);

// A summoned ally (Two Towers 0x0803E110 the summoning star, 0x0802EE54 the ally, its tick 0x0802F0E4
// and AI 0x0800638C): a star 20 pixels left of the caster brings it in on its 4th frame. With no
// target it wanders (10 ticks in a random direction, 8 standing); it takes the first foe within 200
// pixels, walks at it at speed 2.5 (turning aside for 20 ticks past a wall) and attacks within
// `range`, then stands 8 ticks and picks again. Its hits are the caster's level + 50 accuracy and
// level + 10 damage. Foes can't hurt it. After `lifetime` ticks it leaves in another star. One ally
// per player at a time.
typedef struct LegacyAllyDef
{
    // the body walking, standing and attacking, by facing (up .. down; mirrored for the west)
    const u32 *walk;
    const u32 *idle;
    const u32 *attack;
    // the weapon drawn over the body, frame for frame, by facing, or NULL
    const u32 *weaponWalk;
    const u32 *weaponIdle;
    const u32 *weaponAttack;
    // LEGACY_ALLY_HIT_*
    u8 hit;
    // the attack distance, in pixels
    u8 range;
    // its imported palette id (hackLib_fxUsePalette), one per ally
    u8 paletteId;
    LegacyScale lifetime;
} LegacyAllyDef;

// an arrow 9 ticks into the attack (Legolas)
#define LEGACY_ALLY_HIT_ARROW 0
// the body's trigger box 1 on the attack's hit frame (Eomer's soldier)
#define LEGACY_ALLY_HIT_BODY 1
// the weapon's trigger box 0 on the attack's hit frame (Gimli's axe)
#define LEGACY_ALLY_HIT_WEAPON 2

void legacyFx_castAlly(Actor *playerActor, const LegacyAllyDef *def);
// FALSE while the player's ally (or its star) is out.
bool32 legacyFx_allyCanCast(u8 playerIndex);
// Thrown toward a 32-way direction. NULL when none could spawn.
Actor *legacyFx_castThrownToward(Actor *playerActor, const LegacyThrownDef *def, u8 direction32);
// 16.16 velocity of `speed` toward a 32-way direction (clockwise from north).
Vector2Fp16 legacyFx_velocity32(u8 direction32, s32 speed);

// A burst on the caster: every foe within `radius` on the frame it first touches one is stunned
// and takes its damage (either can be zero).
typedef struct LegacyBurstDef
{
    u32 animation;
    u8 radius;
    u8 lifetime;
    LegacyScale stunTicks;
    LegacyScale damage;
    u16 sound;
    // LEGACY_BURST_* options
    u8 flags;
    // the animation frame `onFrame` runs on, once
    u8 onFrameIndex;
    // what the burst sends out on that frame, or NULL
    void (*onFrame)(Actor *fx);
} LegacyBurstDef;

// it stays on the caster
#define LEGACY_BURST_FOLLOW (1 << 0)
// it touches foes on its first tick only, touching one or not
#define LEGACY_BURST_FIRST_TICK (1 << 1)
// it lasts until its animation ends, not `lifetime`
#define LEGACY_BURST_UNTIL_ANIMATION_END (1 << 2)
// it keeps touching foes all its life, not only until it first touches one
#define LEGACY_BURST_EVERY_TICK (1 << 3)

void legacyFx_castBurst(Actor *playerActor, const LegacyBurstDef *def);
// A burst at `level`, for one that is not a cast (it scales with another skill's level).
void legacyFx_castBurstAt(Actor *playerActor, const LegacyBurstDef *def, u8 level);

// One stat a buff raises: value(level) / divisor (divisor 0 means 1).
typedef struct LegacyBuffStat
{
    u8 stat;
    LegacyScale value;
    u8 divisor;
} LegacyBuffStat;

#define LEGACY_BUFF_STATS 3

// A timed buff on the caster: `rising` plays, `burning` for `ticks`, then `fading`, while the caster
// has its stats. One at a time per player.
typedef struct LegacyBuffDef
{
    u32 rising;
    u32 burning;
    u32 fading;
    LegacyScale ticks;
    // unused entries have value {0, 0}
    LegacyBuffStat stats[LEGACY_BUFF_STATS];
    // each player's buff while it is on, two entries
    Actor **active;
} LegacyBuffDef;

bool32 legacyFx_buffCanCast(const LegacyBuffDef *def, u8 playerIndex);
void legacyFx_castBuff(Actor *playerActor, const LegacyBuffDef *def);

// A heal on the caster, with `sparkle` on them.
typedef struct LegacyHealDef
{
    LegacyScale hp;
    u32 sparkle;
    u16 sound;
} LegacyHealDef;

void legacyFx_castHeal(Actor *playerActor, const LegacyHealDef *def);
