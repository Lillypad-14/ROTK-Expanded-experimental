// Summoned allies of the Two Towers actives (Call Legolas, Summon Gimli, Call Eomer).

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "combat.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "rng.h"
#include "sprite.h"

// the summoning star, in the trail lights' palette (kSprites)
#define STAR_ANIMATION 0x08057E50u
#define STAR_OFFSET_X (-(20 << 16))
#define STAR_OFFSET_Y (1 << 16)
// the star's frame the ally appears on
#define STAR_SUMMON_FRAME 3
// keeps an effect alive while its own tick ends it
#define ALLY_HOLD 30

#define ALLY_SPEED 0x28000
#define ALLY_PICK_RADIUS (200 << 16)
#define ALLY_WANDER_TICKS 10
#define ALLY_PAUSE_TICKS 7
#define ALLY_DETOUR_TICKS 20
#define ALLY_ACCURACY_BASE 50
#define ALLY_DAMAGE_BASE 10
// an archer's arrow: ticks into the attack, speed and lifetime (the Two Towers projectile)
#define ALLY_ARROW_WINDUP 9
#define ALLY_ARROW_SPEED 0x78000
#define ALLY_ARROW_LIFETIME 21

// actor_direction8TowardPoint: within the tolerance on both axes
#define DIRECTION8_ARRIVED 8

// an ally's state, in LegacyFxState.phase (the Two Towers actor states it mirrors)
enum
{
    ALLY_PAUSING,  // 5
    ALLY_WANDERING, // 2
    ALLY_CHASING,  // 8
    ALLY_DETOURING, // 0x0E
    ALLY_ATTACKING, // 3
};

// the art of the eight facings: up .. down, then mirrored
static const u8 FacingArt[8] = {0, 1, 2, 3, 4, 3, 2, 1};

// the two sideways directions an ally tries past a wall, per facing (Two Towers 0x083279E4)
static const u8 Detours[8][2] = {{2, 6}, {3, 7}, {4, 0}, {5, 1}, {6, 2}, {7, 3}, {0, 4}, {1, 5}};

static const u32 ArrowSprites[8] = {
    0x08329960u, 0x08329968u, 0x08329970u, 0x08329978u,
    0x08329980u, 0x08329978u, 0x08329970u, 0x08329968u,
};

typedef struct LegacyAlly
{
    // the star while it summons, then the ally's body
    Actor *actor;
    Actor *weapon;
    const LegacyAllyDef *def;
    // the body and weapon animations shown
    u32 shown;
    u32 weaponShown;
    u16 lifetime;
    u8 level;
    // the arrow's 32-way direction
    u8 aim;
} LegacyAlly;

static LegacyAlly Allies[2];

static void starTick(Actor *fx);
static void allyTick(Actor *fx);
static void weaponTick(Actor *fx);

static LegacyAlly *allyOf(Actor *fx)
{
    return &Allies[fx->parentActor->as.combat.playerIndex];
}

static bool32 alive(Actor *actor, ActorCallback tick)
{
    return actor != NULL && actor->tickCallback == tick && !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}

bool32 legacyFx_allyCanCast(u8 playerIndex)
{
    Actor *actor = Allies[playerIndex].actor;

    return !alive(actor, starTick) && !alive(actor, allyTick);
}

static bool32 foeGone(Actor *foe)
{
    return (foe->flags.p & ACTOR_FLAG_PENDING_REMOVE) || !legacyFx_isFoe(foe);
}

static void hitFoe(Actor *attacker, Actor *foe, u8 level)
{
    attacker->behaviorState = NPC_BEHAVIOR_COMBAT_ACTIVE;
    combat_resolveNonPlayerAttackOnEnemy(attacker, foe, level + ALLY_ACCURACY_BASE, level + ALLY_DAMAGE_BASE);
}

static void arrowTick(Actor *fx)
{
    legacyFx_finishTick(fx);
}

static void arrowHit(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other))
    {
        hitFoe(fx, other, LEGACY_FX(fx)->count);
        legacyFx_end(fx);
    }
}

static void shootArrow(Actor *body, LegacyAlly *ally)
{
    Actor *fx = legacyFx_spawn(body->parentActor, 0);

    if (fx == NULL)
    {
        return;
    }
    legacyFx_place(fx, body->coords);
    fx->facing = ally->aim >> 2;
    legacyFx_setSprite(fx, ArrowSprites[fx->facing]);
    if (ally->aim > DIRECTION32_SOUTH)
    {
        fx->oam.matrixIndex |= OAM_MATRIX_H_FLIP;
    }
    else
    {
        fx->oam.matrixIndex &= ~OAM_MATRIX_H_FLIP;
    }
    fx->velocity = legacyFx_velocity32(ally->aim, ALLY_ARROW_SPEED);
    fx->actionStateTimer = ALLY_ARROW_LIFETIME;
    fx->tickCallback = arrowTick;
    fx->triggerCallbacks[0] = arrowHit;
    LEGACY_FX(fx)->count = ally->level;
}

// a melee hit box (the body's or the weapon's) touching something on the attack's hit frame
static void allyHit(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other))
    {
        hitFoe(fx, other, allyOf(fx)->level);
    }
}

// TRUE when a step along `facing` would end in a wall (Two Towers 0x08006D9C)
static bool32 blocked(Actor *fx, Direction8 facing)
{
    Vector2Fp16 step;
    Vector2Fp16 next = fx->coords;

    actor_setVelocityFromDirection8(&step, fx->flags.p, facing, ALLY_SPEED);
    next.x += step.x;
    next.y += step.y;
    return !hackLib_canLandAt(fx, &next);
}

static Direction8 facingToward(Actor *fx, Actor *foe)
{
    Direction8 facing = actor_direction8TowardPoint(fx->coords.x, fx->coords.y, foe->coords.x, foe->coords.y, 0);

    return facing == DIRECTION8_ARRIVED ? fx->facing : facing;
}

static void pause(Actor *fx, u8 ticks)
{
    LEGACY_FX(fx)->phase = ALLY_PAUSING;
    LEGACY_FX(fx)->count = ticks;
}

// the target lost: stand, then pick again
static void dropTarget(Actor *fx)
{
    LEGACY_FX(fx)->target = NULL;
    pause(fx, ALLY_PAUSE_TICKS);
}

static Actor *pickTarget(Actor *fx)
{
    Actor *other = NULL;

    while ((other = actor_iterateInRadius(fx, other, ALLY_PICK_RADIUS)) != NULL)
    {
        if (!foeGone(other))
        {
            return other;
        }
    }
    return NULL;
}

static u32 distanceTo(Actor *fx, Actor *foe)
{
    Vector2Int a = {fx->coords.x >> 16, fx->coords.y >> 16};
    Vector2Int b = {foe->coords.x >> 16, foe->coords.y >> 16};

    return math_vector2_distance(a, b);
}

static void startAttack(Actor *fx, LegacyAlly *ally, Actor *foe)
{
    Vector2Fp16 toward = {foe->coords.x - fx->coords.x, foe->coords.y - fx->coords.y};

    LEGACY_FX(fx)->phase = ALLY_ATTACKING;
    LEGACY_FX(fx)->count = ALLY_ARROW_WINDUP;
    fx->facing = facingToward(fx, foe);
    ally->aim = math_vector2_direction32(toward);
}

static void chase(Actor *fx, LegacyAlly *ally, Actor *foe)
{
    Direction8 facing;
    u8 side;
    u8 detour;

    if (distanceTo(fx, foe) < ally->def->range)
    {
        startAttack(fx, ally, foe);
        return;
    }
    if (foeGone(foe))
    {
        dropTarget(fx);
        return;
    }
    facing = facingToward(fx, foe);
    fx->facing = facing;
    if (!blocked(fx, facing))
    {
        return;
    }
    side = rng_rollUpTo(1);
    detour = Detours[facing][side];
    if (blocked(fx, detour))
    {
        detour = Detours[facing][!side];
        if (blocked(fx, detour))
        {
            fx->facing = detour;
            pause(fx, ALLY_PAUSE_TICKS);
            return;
        }
    }
    fx->facing = detour;
    LEGACY_FX(fx)->phase = ALLY_DETOURING;
    LEGACY_FX(fx)->count = ALLY_DETOUR_TICKS;
}

// One AI step (Two Towers 0x0800638C).
static void think(Actor *fx, LegacyAlly *ally)
{
    LegacyFxState *state = LEGACY_FX(fx);
    Actor *foe = state->target;

    switch (state->phase)
    {
    case ALLY_PAUSING:
    case ALLY_DETOURING:
        if (state->count-- != 0)
        {
            break;
        }
        state->phase = ALLY_WANDERING;
        state->count = ALLY_WANDER_TICKS;
        fx->facing = rng_rollRange(0, 7);
        break;
    case ALLY_WANDERING:
        if (foe != NULL)
        {
            state->phase = ALLY_CHASING;
        }
        else if (state->count == 0 || (state->count--, blocked(fx, fx->facing)))
        {
            pause(fx, ALLY_PAUSE_TICKS);
        }
        break;
    case ALLY_CHASING:
        chase(fx, ally, foe);
        break;
    case ALLY_ATTACKING:
        if (ally->def->hit == LEGACY_ALLY_HIT_ARROW && --state->count == 0)
        {
            shootArrow(fx, ally);
        }
        if (foeGone(foe) || legacyFx_animationEnded(fx))
        {
            dropTarget(fx);
        }
        break;
    }
}

static bool32 moving(u8 phase)
{
    return phase == ALLY_WANDERING || phase == ALLY_CHASING || phase == ALLY_DETOURING;
}

// a walk or a stand starts over at its end
static void show(Actor *fx, u32 *shown, u32 animation, bool32 loop)
{
    if (*shown != animation || (loop && legacyFx_animationEnded(fx)))
    {
        *shown = animation;
        legacyFx_setSprite(fx, animation);
    }
    legacyFx_faceSprite(fx);
}

// the weapon over the body, on the body's frame
static void dressWeapon(Actor *fx, LegacyAlly *ally, u8 phase)
{
    const LegacyAllyDef *def = ally->def;
    Actor *weapon = ally->weapon;
    const u32 *bank = moving(phase) ? def->weaponWalk : phase == ALLY_ATTACKING ? def->weaponAttack : def->weaponIdle;
    u16 frames;

    if (!alive(weapon, weaponTick))
    {
        return;
    }
    weapon->coords = fx->coords;
    weapon->facing = fx->facing;
    show(weapon, &ally->weaponShown, bank[FacingArt[fx->facing]], FALSE);
    frames = weapon->spriteAnimation->frames->frameCount;
    sprite_setAnimationFrame(weapon, fx->currentAnimationFrame < frames ? fx->currentAnimationFrame : frames - 1);
    if (phase == ALLY_ATTACKING && def->hit == LEGACY_ALLY_HIT_WEAPON)
    {
        weapon->flags.p |= ACTOR_FLAG_COLLISION_CHECK;
    }
    else
    {
        weapon->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
}

static void leave(Actor *fx, LegacyAlly *ally)
{
    Actor *star = legacyFx_spawn(fx->parentActor, STAR_ANIMATION);

    if (star != NULL)
    {
        legacyFx_place(star, fx->coords);
        star->tickCallback = starTick;
        star->actionStateTimer = ALLY_HOLD;
        // a leaving star summons nothing
        LEGACY_FX(star)->phase = 1;
    }
    if (alive(ally->weapon, weaponTick))
    {
        legacyFx_end(ally->weapon);
    }
    legacyFx_end(fx);
}

// The ally's tick (Two Towers 0x0802F0E4).
static void allyTick(Actor *fx)
{
    LegacyAlly *ally = allyOf(fx);
    LegacyFxState *state = LEGACY_FX(fx);
    const LegacyAllyDef *def = ally->def;
    u8 phase;

    if (--ally->lifetime == 0)
    {
        leave(fx, ally);
        legacyFx_finishTick(fx);
        return;
    }
    if (state->target == NULL)
    {
        state->target = pickTarget(fx);
    }
    think(fx, ally);
    phase = state->phase;
    // a spawned effect with tile collision ends at a wall: the ally stops there instead
    if (moving(phase) && !blocked(fx, fx->facing))
    {
        actor_setVelocityFromDirection8(&fx->velocity, fx->flags.p, fx->facing, ALLY_SPEED);
    }
    else
    {
        fx->velocity.x = 0;
        fx->velocity.y = 0;
    }
    show(fx, &ally->shown,
         (moving(phase) ? def->walk : phase == ALLY_ATTACKING ? def->attack : def->idle)[FacingArt[fx->facing]],
         phase != ALLY_ATTACKING);
    if (phase == ALLY_ATTACKING && def->hit == LEGACY_ALLY_HIT_BODY)
    {
        fx->flags.p |= ACTOR_FLAG_COLLISION_CHECK;
    }
    else
    {
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
    fx->actionStateTimer = ALLY_HOLD;
    legacyFx_finishTick(fx);
    if (def->weaponIdle != NULL)
    {
        dressWeapon(fx, ally, phase);
    }
}

static void weaponTick(Actor *fx)
{
    if (!alive(allyOf(fx)->actor, allyTick))
    {
        legacyFx_end(fx);
    }
    else
    {
        fx->actionStateTimer = ALLY_HOLD;
    }
    legacyFx_finishTick(fx);
}

// Its own palette slot, as it outlives the effects sharing theirs. The weapon draws in the body's palette.
static void usePalette(Actor *fx, LegacyAlly *ally)
{
    const SpriteAnimation *art = legacyFx_sprite(ally->def->idle[0]);

    if (art != NULL)
    {
        hackLib_fxUsePalette(fx, art->palette, ally->def->paletteId);
    }
}

static void summon(Actor *star, LegacyAlly *ally)
{
    const LegacyAllyDef *def = ally->def;
    Actor *fx = legacyFx_spawn(star->parentActor, 0);
    Actor *weapon;

    if (fx == NULL)
    {
        return;
    }
    legacyFx_place(fx, star->coords);
    fx->facing = DIRECTION8_SOUTH;
    fx->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
    fx->tickCallback = allyTick;
    fx->triggerCallbacks[1] = def->hit == LEGACY_ALLY_HIT_BODY ? allyHit : NULL;
    fx->actionStateTimer = ALLY_HOLD;
    ally->actor = fx;
    ally->shown = 0;
    ally->weaponShown = 0;
    ally->weapon = NULL;
    pause(fx, ALLY_PAUSE_TICKS);
    // its own slot before its first sprite, which would load its colors over the shared effects'
    usePalette(fx, ally);
    show(fx, &ally->shown, def->idle[FacingArt[fx->facing]], TRUE);
    if (def->weaponIdle == NULL)
    {
        return;
    }
    weapon = legacyFx_spawn(star->parentActor, 0);
    if (weapon == NULL)
    {
        return;
    }
    legacyFx_place(weapon, fx->coords);
    weapon->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
    weapon->tickCallback = weaponTick;
    weapon->triggerCallbacks[0] = def->hit == LEGACY_ALLY_HIT_WEAPON ? allyHit : NULL;
    weapon->actionStateTimer = ALLY_HOLD;
    ally->weapon = weapon;
    usePalette(weapon, ally);
}

static void starTick(Actor *fx)
{
    LegacyAlly *ally = allyOf(fx);

    if (LEGACY_FX(fx)->phase == 0 && fx->currentAnimationFrame >= STAR_SUMMON_FRAME)
    {
        LEGACY_FX(fx)->phase = 1;
        summon(fx, ally);
    }
    fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : ALLY_HOLD;
    legacyFx_finishTick(fx);
}

void legacyFx_castAlly(Actor *playerActor, const LegacyAllyDef *def)
{
    LegacyAlly *ally = &Allies[playerActor->as.combat.playerIndex];
    u8 level = legacyFx_castingLevel(playerActor);
    Actor *star = legacyFx_spawn(playerActor, STAR_ANIMATION);
    Vector2Fp16 at = playerActor->coords;

    if (star == NULL)
    {
        return;
    }
    at.x += STAR_OFFSET_X;
    at.y += STAR_OFFSET_Y;
    legacyFx_place(star, at);
    star->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
    star->tickCallback = starTick;
    star->actionStateTimer = ALLY_HOLD;
    ally->actor = star;
    ally->def = def;
    ally->level = level;
    ally->lifetime = legacyScale(def->lifetime, level);
}

#endif
