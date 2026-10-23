#pragma once

#include "actor.h"
#include "types.h"
#include "vector2.h"

enum FxParticleFlag
{
    FX_PARTICLE_FLAG_LOOP_ANIMATION = 1 << 2,
    FX_PARTICLE_FLAG_DIRECTIONAL_OSCILLATION = 1 << 3,
    FX_PARTICLE_FLAG_ORBIT = 1 << 5,
    FX_PARTICLE_FLAG_ALWAYS_VISIBLE = 1 << 7,
    FX_PARTICLE_FLAG_QUEUE_EARLIER = 1 << 14,
    FX_PARTICLE_FLAG_RENDER_QUEUED = 1 << 15,
};

/** Decoded prefix of a pooled particle record. */
typedef struct FxParticle
{
    struct FxParticle *next;
    struct FxParticle *previous;
    u32 field_0x8;
    const SpriteAnimation *animation;
    Vector2Fp16 position;
    Vector2Fp16 renderPosition;
    Vector2Fp16 velocity;
    OamEntry oam;
    u8 field_0x30[12];
    s16 lifetime;
    u16 flags;
    u16 field_0x40;
    s16 animationElapsed;
    s8 animationFrameDuration;
    s8 animationFrame;
    u8 updateMode;
    Direction8 direction;
    Angle8 phase;
    u8 amplitude;
    u8 phaseStep;
    u8 field_0x4b;
} FxParticle;

void fx_buildRenderQueues(void);
s32 fx_renderLayer(u8 priority);
s32 fx_tickAndRenderParticle(FxParticle *particle);
void fx_particle_tickMode0(FxParticle *particle);
void fx_particle_tickMode1(FxParticle *particle);
void fx_particle_tickMode2(FxParticle *particle);
s32 fx_drawParticle(FxParticle *particle);
u8 fx_classifyParticleVisibility(FxParticle *particle);

/** One row of the FX descriptor table. */
typedef struct FxDesc
{
    u8 anim[0x10]; // SpriteAnimation
    u8 flags;      // bit 1 anchors via actor_getOffsetPosition, bit 2 anchors on the animation frame
    s8 offX;
    u8 offY;
    u8 pad;
} FxDesc;

/** fx_spawnOnActor type ids - rows of the FX descriptor table @0x08287b7c (stride 0x14). */
#define FX_TYPE_HIT_BURST 0x16

/** the on-kill AoE flash spawned on the killer by combat_sunburstOnKill/combat_nightburstOnKill */
#define FX_TYPE_AOE_BURST 0

// AoE-burst variant / fx_allocEmitter kind by proc
#define FX_AOE_BURST_SUN 8
#define FX_AOE_BURST_NIGHT 7

/** spawned by Gimli's shield bubble when it absorbs part of a hit */
#define FX_TYPE_SHIELD_ABSORB 9

/** written to that shield-absorb FX actor's type */
#define FX_SHIELD_ABSORB_UNK_PARAM 0x4d

/** variant values (sprite/palette select nibble) for FX_TYPE_HIT_BURST */
enum FxBurstVariant
{
    FX_BURST_TRIPLE = 6,
    FX_BURST_FIRE = 7,
    FX_BURST_VENOM = 8,
};

/**
 * written to the spawned burst FX actor's type by the Fireburst / Venomous melee hit effects
 */
#define FX_BURST_UNK_PARAM 0x68

Actor *fx_spawnOnActor(Actor *parent, u8 fxType, u8 variant, u8 alt);

/** Spawn attached FX with a timed animation sequence. */
Actor *fx_spawnTimedOnActor(Actor *parent, s32 duration, u8 fxType, u8 variant, u8 alt);

/** Advance timed attached FX. Returns TRUE when the effect finishes. */
bool32 fx_tickTimedAttachment(Actor *fx);

/**
 * Tick callback for attached FX: follows the parent, self-destructs when the
 * parent actor is gone.
 */
u32 fx_tickAttachment(Actor *fx);

/**
 * Spawn the boxed interaction FX (type 0xe: hand-built trigger box) at a
 * position; variant selects the behavior. Called for a chest's on-open spawn.
 * Provisional.
 */
void fx_spawnTriggerFx(s32 x, s32 y, u8 variant);

/**
 * Transient particle-burst emitter, pooled (max 14 live). Its bulk is an un-decoded particle
 * descriptor filled by fx_allocEmitter from the descriptor tables; only .target and the handful of
 * fields the AoE on-kill procs (combat_sunburstOnKill/combat_nightburstOnKill) set afterward are
 * broken out.
 */
typedef struct FxEmitter
{
    u8 field_0x0[8];

    /** actor the emitter is anchored to */
    Actor *target;

    u32 field_0xc;
    u8 field_0x10[8];

    /** world position the burst plays at (fp16_16); anchored on .target's animation frame */
    Vector2Fp16 position;

    u32 field_0x20;
    void *field_0x24;
    u16 field_0x28;
    u16 field_0x2a;

    /** emitter kind (fx_allocEmitter's first arg) */
    u8 kind;

    u8 field_0x2d;
    u16 field_0x2e; // flag bits; the burst procs OR in 0x4082
    u16 field_0x30;
    u8 field_0x32[2];
    u16 field_0x34;
    u8 field_0x36;
    u8 field_0x37;
    u8 field_0x38;
    u8 field_0x39;
    u16 field_0x3a;
    u8 field_0x3c[3];
    u8 field_0x3f;
    u8 field_0x40[4];
} FxEmitter;
/** Anchor the emitter's world position on the actor's current animation frame. */
static inline void fx_setEmitterPosition(FxEmitter *emitter, Actor *actor, Vector2Fp16 *out)
{
    // the callee fills raw frame offsets; they become fp16_16 after the scale below
    actor_getAnimationFrame(actor, (u32 *)out);
    out->x = actor->coords.x;
    out->y = actor->coords.y - (out->y << 15);
    emitter->position = *out;
}

/**
 * Allocate and preload a pooled FxEmitter for the given kind (0 when the 14-slot pool is full).
 * The two extra args index the particle descriptor tables.
 */
FxEmitter *fx_allocEmitter(u8 kind, u32 group, u32 variant);
void fx_freeEmitter(FxEmitter *emitter);
void fx_freeParticle(Actor *particle);
void mission_unknown08039198(void);

/** Tick every live emitter in the pool, freeing the ones that expired. */
void fx_tickEmitters(void);

Actor *fx_attachStatusEffect(Actor *parent, Actor *target, u16 duration, u8 kind, u8 arg4, u8 arg5,
                             u8 flagBit);

void fx_statusEffect_tick(Actor *fx);

void fx_resetStunSfxCooldown(void);
void fx_tickStunSfxCooldown(void);
Actor *fx_unknown08044000(Actor *parent, u32 a, u8 variant, u8 facing, u32 b, u32 c);
Actor *fx_unknown08043d74(Actor *parent, u32 a, u32 b, u32 c, u32 d, s32 e);
