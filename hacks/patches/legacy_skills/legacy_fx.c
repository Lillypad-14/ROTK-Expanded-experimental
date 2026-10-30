#include "patches/legacy_skills/legacy_fx.h"
#include "patches/legacy_skills/legacy_skills.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "combat.h"
#include "fx.h"
#include "gba.h"
#include "gfx.h"
#include "stats.h"
#include "lib/pc_host.h"
#include "memory.h"
#include "player.h"
#include "sfx.h"
#include "variables.h"

_Static_assert(sizeof(LegacyFxState) <= sizeof(((Actor *)0)->as.combat.head), "LegacyFxState");

// the projectile spawner's arguments for a thrown knife: no target, its palette bank, no slot
#define PROJECTILE_ANIMATION_SET 1
#define PROJECTILE_AIMED 1
#define PROJECTILE_PALETTE_BANK 2
#define PROJECTILE_NO_SLOT -1
// ticks an effect waits on its animation before the tick ends it
#define EFFECT_HOLD 30

u8 legacyFx_castingLevel(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    u32 row = PLAYER(playerIndex).castingActiveSkill;

    if (legacySkills_loaded(playerIndex) && row >= SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW &&
        row < SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + SKILL_EXPANSION_LEGACY_ACTIVE_COUNT)
    {
        return legacySkills_level(legacySkills_passiveCount() + row - SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW);
    }
    return *skillExpansion_activeLevel(playerIndex, row);
}

const SpriteAnimation *legacyFx_sprite(u32 address)
{
    const TwoTowersData *data = hackLib_twoTowersData();

    if (data == NULL || data->sprites == NULL)
    {
        return NULL;
    }
    for (u32 i = 0; i < data->spriteCount; i++)
    {
        if (data->sprites[i * 2] == address)
        {
            return (const SpriteAnimation *)data->sprites[i * 2 + 1];
        }
    }
    return NULL;
}

/**
 * The spawner leaves its own animation script running, which would drive the frames and keep the
 * animation from ever ending, so this clears it.
 */
void legacyFx_setSprite(Actor *fx, u32 animation)
{
    const SpriteAnimation *sprite = animation != 0 ? legacyFx_sprite(animation) : NULL;

    if (sprite != NULL)
    {
        fx->animationScriptCursor = NULL;
        sprite_setAnimation(fx, sprite);
    }
}

Actor *legacyFx_spawn(Actor *playerActor, u32 animation)
{
    Actor *fx = fx_unknown08043d74(playerActor, 0, PROJECTILE_ANIMATION_SET, PROJECTILE_AIMED,
                                   PROJECTILE_PALETTE_BANK, PROJECTILE_NO_SLOT);

    if (fx == NULL)
    {
        return NULL;
    }
    legacyFx_setSprite(fx, animation);
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    fx->triggerCallbacks[0] = NULL;
    fx->parentActor = playerActor;
    // the spawner leaves the scratch as its last user did
    memory_memset(LEGACY_FX(fx), 0, sizeof(LegacyFxState));
    return fx;
}

static void onCasterTick(Actor *fx)
{
    fx->coords = fx->parentActor->coords;
    fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : EFFECT_HOLD;
    actor_unknown08045584(fx);
}

void legacyFx_spawnOnCaster(Actor *playerActor, u32 animation)
{
    Actor *fx;

    if (legacyFx_sprite(animation) == NULL)
    {
        return;
    }
    fx = legacyFx_spawn(playerActor, animation);
    if (fx != NULL)
    {
        fx->tickCallback = onCasterTick;
    }
}

bool32 legacyFx_isFoe(Actor *other)
{
    return (other->flags.p & LEGACY_FOE_HITTABLE_FLAGS) != 0;
}

bool32 legacyFx_animationEnded(Actor *fx)
{
    return (fx->flags.p & ACTOR_FLAG_ANIMATION_END) != 0;
}

void legacyFx_faceSprite(Actor *fx)
{
    if (fx->facing >= DIRECTION8_SOUTHWEST && fx->facing <= DIRECTION8_NORTHWEST)
    {
        fx->oam.matrixIndex |= OAM_MATRIX_H_FLIP;
    }
    else
    {
        fx->oam.matrixIndex &= ~OAM_MATRIX_H_FLIP;
    }
}

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

bool32 legacyFx_onScreen(Actor *fx, s32 margin)
{
    Vector2Int camera;
    s32 x = fx->coords.x >> 16, y = fx->coords.y >> 16;

    gfx_getCameraPosition(&camera, fx->parentActor->as.combat.playerIndex);
    return x >= camera.x - margin && x <= camera.x + SCREEN_WIDTH + margin && y >= camera.y - margin &&
           y <= camera.y + SCREEN_HEIGHT + margin;
}

// Actor.as words the spawner seeds with its spawn position
#define SPAWN_POSITION_OFFSET 0x20

void legacyFx_place(Actor *fx, Vector2Fp16 pos)
{
    fx->coords = pos;
    fx->renderCoords = pos;
    *(Vector2Fp16 *)((u8 *)&fx->as + SPAWN_POSITION_OFFSET) = pos;
}

void legacyFx_end(Actor *fx)
{
    fx->actionState = ACTOR_STATE_UNKNOWN_04;
    fx->actionStateTimer = 0;
}

void legacyFx_finishTick(Actor *fx)
{
    if (fx->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    actor_unknown08045584(fx);
}

void legacyFx_playSound(u16 sound)
{
    if (sound != LEGACY_SOUND_NONE)
    {
        sfx_play(sound);
    }
}

// a projectile's phase
#define PROJECTILE_FLYING 0
// it touched a foe and bursts next tick
#define PROJECTILE_HIT 1
// its blast collides on this frame only
#define PROJECTILE_BURSTING 2
// it plays its impact on the foe
#define PROJECTILE_IMPACT 3

static void projectileBlastHit(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other))
    {
        combat_applyPveRangedHit(fx->parentActor, other, LEGACY_FX(fx)->amount);
    }
}

static void projectileHit(Actor *fx, Actor *other)
{
    LegacyFxState *state = LEGACY_FX(fx);
    const LegacyProjectileDef *def = state->def;

    if (!legacyFx_isFoe(other) || state->phase != PROJECTILE_FLYING)
    {
        return;
    }
    if (def->blast != 0)
    {
        state->phase = PROJECTILE_HIT;
        return;
    }
    combat_applyPveRangedHit(fx->parentActor, other, state->amount);
    state->phase = PROJECTILE_IMPACT;
    fx->triggerCallbacks[0] = NULL;
    fx->renderFollowTarget = other;
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    legacyFx_setSprite(fx, def->impact);
}

static void projectileTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    const LegacyProjectileDef *def = state->def;

    if (state->phase == PROJECTILE_HIT)
    {
        state->phase = PROJECTILE_BURSTING;
        fx->velocity.x = 0;
        fx->velocity.y = 0;
        legacyFx_setSprite(fx, def->blast);
        fx->triggerCallbacks[0] = projectileBlastHit;
        fx->actionStateTimer = def->lifetime;
        legacyFx_playSound(def->blastSound);
    }
    else if (state->phase == PROJECTILE_BURSTING || state->phase == PROJECTILE_IMPACT)
    {
        if (state->phase == PROJECTILE_BURSTING)
        {
            fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
        }
        fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : def->lifetime;
    }
    legacyFx_finishTick(fx);
}

void legacyFx_castProjectile(Actor *playerActor, const LegacyProjectileDef *def)
{
    Actor *fx = legacyFx_spawn(playerActor, 0);
    LegacyFxState *state;

    if (fx == NULL)
    {
        return;
    }
    state = LEGACY_FX(fx);
    legacyFx_setSprite(fx, def->sprites[fx->facing & 7]);
    legacyFx_faceSprite(fx);
    actor_setVelocityFromDirection8(&fx->velocity, fx->flags.p, fx->facing, def->speed);
    fx->actionStateTimer = def->lifetime;
    fx->tickCallback = projectileTick;
    fx->triggerCallbacks[0] = projectileHit;
    state->def = def;
    state->amount = legacyScale(def->damage, legacyFx_castingLevel(playerActor));
    state->phase = PROJECTILE_FLYING;
    legacyFx_playSound(def->sound);
}

static void thrownTick(Actor *fx)
{
    legacyFx_finishTick(fx);
}

static u8 thrownStat(const LegacyThrownDef *def)
{
    return def->bonusStat != 0 ? def->bonusStat : STAT_DAMAGE_FLAT;
}

static void thrownHit(Actor *fx, Actor *other)
{
    const LegacyThrownDef *def = LEGACY_FX(fx)->def;
    Actor *caster = fx->parentActor;
    u8 playerIndex = caster->as.combat.playerIndex;

    if (!legacyFx_isFoe(other) || other->actionState == ACTOR_STATE_HURT)
    {
        return;
    }
    PLAYER_STAT(playerIndex, thrownStat(def)) += LEGACY_FX(fx)->amount;
    combat_resolvePveAttack(caster, other);
    PLAYER_STAT(playerIndex, thrownStat(def)) -= LEGACY_FX(fx)->amount;
    if (def->onHit != NULL)
    {
        def->onHit(fx, other);
    }
    if ((other->flags.p & ACTOR_FLAG_NO_FLINCH) || (def->flags & LEGACY_THROWN_STOP_ON_HIT))
    {
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
        legacyFx_end(fx);
    }
}

// sin(k * 90 / 8 degrees) in 16.16, the values of The Two Towers' 32-way velocity table
static const s32 QuarterSines[9] = {0, 12785, 25079, 36409, 46341, 54491, 60547, 64276, 65536};

static s32 sine32(u32 direction32)
{
    u32 k = direction32 & 31;

    if (k <= 8)
    {
        return QuarterSines[k];
    }
    if (k <= 16)
    {
        return QuarterSines[16 - k];
    }
    if (k <= 24)
    {
        return -QuarterSines[k - 16];
    }
    return -QuarterSines[32 - k];
}

Vector2Fp16 legacyFx_velocity32(u8 direction32, s32 speed)
{
    Vector2Fp16 v;

    v.x = (s32)(((s64)sine32(direction32) * speed) >> 16);
    v.y = (s32)(((s64)-sine32(direction32 + 8) * speed) >> 16);
    return v;
}

Actor *legacyFx_castThrownToward(Actor *playerActor, const LegacyThrownDef *def, u8 direction32)
{
    Actor *fx = legacyFx_spawn(playerActor, 0);

    if (fx == NULL)
    {
        return NULL;
    }
    direction32 &= 31;
    fx->facing = direction32 >> 2;
    legacyFx_setSprite(fx, def->sprites[fx->facing]);
    // mirrored past south, by the 32-way direction
    if (direction32 > DIRECTION32_SOUTH)
    {
        fx->oam.matrixIndex |= OAM_MATRIX_H_FLIP;
    }
    else
    {
        fx->oam.matrixIndex &= ~OAM_MATRIX_H_FLIP;
    }
    fx->velocity = legacyFx_velocity32(direction32, def->speed);
    if (def->flags & LEGACY_THROWN_START_AHEAD)
    {
        fx->coords.x += fx->velocity.x;
        fx->coords.y += fx->velocity.y;
    }
    fx->actionStateTimer = def->lifetime;
    fx->tickCallback = thrownTick;
    fx->triggerCallbacks[0] = thrownHit;
    LEGACY_FX(fx)->def = def;
    LEGACY_FX(fx)->amount = legacyScale(def->bonus, legacyFx_castingLevel(playerActor));
    return fx;
}

void legacyFx_castThrown(Actor *playerActor, const LegacyThrownDef *def)
{
    if (legacyFx_castThrownToward(playerActor, def, playerActor->facing * 4) != NULL)
    {
        legacyFx_playSound(def->sound);
    }
}

// a burst's phase
#define BURST_LOOKING 0
#define BURST_TOUCHED 1

static void burstTick(Actor *fx)
{
    const LegacyBurstDef *def = LEGACY_FX(fx)->def;

    // it acts on what it touches on the frame it first touches a foe, then stops colliding
    if ((LEGACY_FX(fx)->phase == BURST_TOUCHED && !(def->flags & LEGACY_BURST_EVERY_TICK)) ||
        (def->flags & LEGACY_BURST_FIRST_TICK))
    {
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
    if (def->flags & LEGACY_BURST_FOLLOW)
    {
        fx->coords = fx->parentActor->coords;
    }
    if (def->flags & LEGACY_BURST_UNTIL_ANIMATION_END)
    {
        fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : def->lifetime;
    }
    legacyFx_finishTick(fx);
}

static void burstHit(Actor *fx, Actor *other)
{
    LegacyFxState *state = LEGACY_FX(fx);
    const LegacyBurstDef *def = state->def;
    s32 damage;

    if (!legacyFx_isFoe(other))
    {
        return;
    }
    if (state->amount != 0)
    {
        other->actionState = ACTOR_STATE_STUNNED;
        other->actionStateTimer = state->amount;
        other->field_0x64[0] |= 2;
    }
    damage = legacyScale(def->damage, state->count);
    if (damage > 0)
    {
        combat_applyPveRangedHit(fx->parentActor, other, damage);
    }
    state->phase = BURST_TOUCHED;
}

void legacyFx_castBurst(Actor *playerActor, const LegacyBurstDef *def)
{
    legacyFx_castBurstAt(playerActor, def, legacyFx_castingLevel(playerActor));
}

// The burst keeps its level in its count.
void legacyFx_castBurstAt(Actor *playerActor, const LegacyBurstDef *def, u8 level)
{
    Actor *fx = legacyFx_spawn(playerActor, def->animation);
    LegacyFxState *state;

    if (fx == NULL)
    {
        return;
    }
    state = LEGACY_FX(fx);
    fx->renderFollowTarget = NULL;
    fx->tickCallback = burstTick;
    fx->triggerCallbacks[0] = burstHit;
    fx->actionStateTimer = def->lifetime;
    fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY;
    fx->interactionRadius = def->radius;
    state->def = def;
    state->amount = legacyScale(def->stunTicks, level);
    state->count = level;
    state->phase = BURST_LOOKING;
    legacyFx_playSound(def->sound);
}

// a buff's phase
#define BUFF_RISING 0
#define BUFF_BURNING 1
#define BUFF_FADING 2
#define BUFF_SPENT 3
// ticks the rising and fading animations are given to end
#define BUFF_HOLD 30

static void buffTick(Actor *fx);

bool32 legacyFx_buffCanCast(const LegacyBuffDef *def, u8 playerIndex)
{
    Actor *buff = def->active[playerIndex];

    return buff == NULL || buff->tickCallback != buffTick || (buff->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}

// The buff's stats, by the level it keeps in its count.
static void buffStats(Actor *fx, s32 sign)
{
    const LegacyBuffDef *def = LEGACY_FX(fx)->def;
    u8 playerIndex = fx->parentActor->as.combat.playerIndex;

    for (u32 i = 0; i < LEGACY_BUFF_STATS; i++)
    {
        const LegacyBuffStat *stat = &def->stats[i];
        s32 value = legacyScale(stat->value, LEGACY_FX(fx)->count) / (stat->divisor != 0 ? stat->divisor : 1);

        PLAYER_STAT(playerIndex, stat->stat) += sign * value;
    }
}

// The bonus goes when the buff does, however it ends.
static void buffEnd(Actor *fx)
{
    if (LEGACY_FX(fx)->phase != BUFF_SPENT)
    {
        buffStats(fx, -1);
        LEGACY_FX(fx)->phase = BUFF_SPENT;
    }
    legacyFx_end(fx);
}

static void buffTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    const LegacyBuffDef *def = state->def;

    fx->coords = fx->parentActor->coords;
    if (state->phase == BUFF_RISING)
    {
        fx->actionStateTimer = legacyScale(def->ticks, state->count);
        if (legacyFx_animationEnded(fx))
        {
            state->phase = BUFF_BURNING;
            legacyFx_setSprite(fx, def->burning);
        }
    }
    else if (state->phase == BUFF_BURNING)
    {
        if (fx->actionStateTimer <= 1)
        {
            state->phase = BUFF_FADING;
            legacyFx_setSprite(fx, def->fading);
            fx->actionStateTimer = BUFF_HOLD;
        }
    }
    else if (state->phase == BUFF_FADING)
    {
        fx->actionStateTimer = BUFF_HOLD;
        if (legacyFx_animationEnded(fx))
        {
            buffEnd(fx);
        }
    }
    if (fx->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        buffEnd(fx);
    }
    actor_unknown08045584(fx);
}

void legacyFx_castBuff(Actor *playerActor, const LegacyBuffDef *def)
{
    u8 level = legacyFx_castingLevel(playerActor);
    Actor *fx = legacyFx_spawn(playerActor, def->rising);

    if (fx == NULL)
    {
        return;
    }
    fx->tickCallback = buffTick;
    fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    fx->actionStateTimer = BUFF_HOLD;
    LEGACY_FX(fx)->def = def;
    LEGACY_FX(fx)->phase = BUFF_RISING;
    LEGACY_FX(fx)->count = level;
    buffStats(fx, 1);
    def->active[playerActor->as.combat.playerIndex] = fx;
}

void legacyFx_castHeal(Actor *playerActor, const LegacyHealDef *def)
{
    player_addHp(playerActor->as.combat.playerIndex,
                 legacyScale(def->hp, legacyFx_castingLevel(playerActor)), FALSE);
    legacyFx_spawnOnCaster(playerActor, def->sparkle);
    legacyFx_playSound(def->sound);
}

#endif
