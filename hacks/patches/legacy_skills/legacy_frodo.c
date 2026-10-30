// Frodo's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"
#include "patches/legacy_skills/legacy_skills.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "combat.h"
#include "lib/movement.h"
#include "player.h"
#include "scene.h"
#include "skill.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

// Hobbit Appetite (Two Towers 0x0803C368, 0x0801E3EC): heals 25 + 5 per level for 8 + 2 per level
// Spirit, with Magical Healing's sparkle on Frodo.
static const LegacyHealDef HobbitAppetite = {
    .hp = {25, 5},
    .sparkle = 0x080501D8u,
    .sound = SFX_HERB_PICKUP,
};

static void castHobbitAppetite(Actor *playerActor)
{
    legacyFx_castHeal(playerActor, &HobbitAppetite);
}

// Knife Throw (Two Towers 0x0803BE30, the knife 0x08017694, its hit 0x08011004): for 2 Spirit, the
// knife flies 21 ticks and hits every foe it passes with Frodo's own melee hit. The hit adds nothing
// per level (the description says +1).
static const u32 KnifeSprites[8] = {
    0x08328148u, 0x08328150u, 0x08328158u, 0x08328160u,
    0x08328168u, 0x08328160u, 0x08328158u, 0x08328150u,
};

static const LegacyThrownDef Knife = {
    .sprites = KnifeSprites,
    .speed = 0x50000,
    .lifetime = 21,
    .sound = LEGACY_SOUND_NONE,
};

static void castKnifeThrow(Actor *playerActor)
{
    legacyFx_castThrown(playerActor, &Knife);
}

// A dash: for its ticks the caster moves along their facing at their own move speed. With a damage
// bonus, the first foe it touches takes the caster's melee hit with the bonus and the dash ends.
#define DASH_TICKS 13
#define DASH_REACH 16
#define DASH_HITS 1
#define DASH_STOPPED 2

static void dashTick(Actor *fx)
{
    Actor *caster = fx->parentActor;

    if (LEGACY_FX(fx)->phase == 0)
    {
        Vector2Fp16 step;
        Vector2Fp16 next;

        // the cast pose holds the caster still, so the dash moves them itself, up to a wall
        actor_setVelocityFromDirection8(&step, caster->flags.p, caster->facing, caster->moveSpeed);
        next.x = caster->coords.x + step.x;
        next.y = caster->coords.y + step.y;
        if (hackLib_canLandAt(caster, &next))
        {
            caster->renderCoords.x += step.x;
            caster->renderCoords.y += step.y;
            caster->coords = next;
        }
        else
        {
            LEGACY_FX(fx)->phase = DASH_STOPPED;
        }
    }
    fx->coords = caster->coords;
    legacyFx_finishTick(fx);
}

static void dashHit(Actor *fx, Actor *other)
{
    Actor *caster = fx->parentActor;
    u8 playerIndex = caster->as.combat.playerIndex;

    if (!legacyFx_isFoe(other) || LEGACY_FX(fx)->phase != 0)
    {
        return;
    }
    PLAYER_STAT(playerIndex, STAT_DAMAGE_FLAT) += LEGACY_FX(fx)->amount;
    combat_resolvePveAttack(caster, other);
    PLAYER_STAT(playerIndex, STAT_DAMAGE_FLAT) -= LEGACY_FX(fx)->amount;
    LEGACY_FX(fx)->phase = DASH_HITS;
    legacyFx_end(fx);
}

static void dash(Actor *playerActor, bool32 hits, s32 bonus)
{
    Actor *fx = legacyFx_spawn(playerActor, 0);

    if (fx == NULL)
    {
        return;
    }
    fx->flags.p &= ~(ACTOR_FLAG_RENDER | ACTOR_FLAG_TILE_COLLISION);
    fx->tickCallback = dashTick;
    fx->actionStateTimer = DASH_TICKS;
    LEGACY_FX(fx)->phase = 0;
    if (hits)
    {
        fx->triggerCallbacks[0] = dashHit;
        fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY | ACTOR_FLAG_COLLISION_CHECK;
        fx->interactionRadius = DASH_REACH;
        LEGACY_FX(fx)->amount = bonus;
    }
    else
    {
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
}

// Escape (Two Towers 0x0803BDD0, its roll 0x0801E1DC, the stun 0x080170E8, its tick 0x08017220 and
// hit 0x080112A8): for 3 Spirit (the description says 3 per level), Frodo rolls along his facing at
// his move speed while a stun on him, for 13 ticks, stuns every foe it touches for 30 + 6 per level.
static const LegacyBurstDef EscapeStun = {
    .animation = 0x08050088u,
    .radius = 75,
    .lifetime = 13,
    .stunTicks = {30, 6},
    .sound = LEGACY_SOUND_NONE,
    .flags = LEGACY_BURST_FOLLOW | LEGACY_BURST_EVERY_TICK,
};

static void castEscape(Actor *playerActor)
{
    legacyFx_castBurst(playerActor, &EscapeStun);
    dash(playerActor, FALSE, 0);
}

// Charge (Two Towers 0x0803BCF0, its run 0x0801E31C, its attack 0x0801E484): for 4 Spirit (the
// description says 2 per level), Frodo runs along his facing at his move speed and attacks the first
// foe he reaches with his melee hit, +2 damage per level (the description's, not yet found in the
// code).
#define CHARGE_DAMAGE_PER_LEVEL 2

static void castCharge(Actor *playerActor)
{
    dash(playerActor, TRUE, CHARGE_DAMAGE_PER_LEVEL * legacyFx_castingLevel(playerActor));
}

// The One Ring (Two Towers 0x0803C310, the ring 0x080172E4 and its tick 0x080174FC): for 10 Spirit and
// 35 corruption, Frodo is unseen for 300 + 60 per level ticks with +4 damage, the world in the Ring's
// shadow. It is ROTK's ring (unseen flag, shadow world, the ring's tick) with
// The Two Towers' numbers. Not ported: foes within 75 pixels standing idle for 90 ticks when he puts
// it on (ROTK's foes already lose an unseen hero), and the ring sprite on him, which The Two Towers
// shows only on a co-op partner's screen.
#define ONE_RING_DAMAGE 4
#define ONE_RING_CORRUPTION 35
// ROTK's ring actor and its shadow world (skill_active_frodoTheOneRing)
#define ONE_RING_ACTOR_TYPE 0x5f
#define ONE_RING_ACTOR_FLAGS 0x10008
#define ONE_RING_ON 1
#define ONE_RING_UNSEEN_FLAG (1 << 23)
#define ONE_RING_STATE_FLAG 8
#define ONE_RING_TRANSITION 8
// a co-op partner's ring: Frodo drawn see-through on the other screen
#define ONE_RING_PARTNER_SPRITE_FLAG 0x20

static const LegacyScale OneRingTicks = {300, 60};

static void castTheOneRing(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    Actor *ring;

    if (PLAYER(playerIndex).ringActor != NULL)
    {
        return;
    }
    ring = actor_allocMain();
    if (ring == NULL)
    {
        return;
    }
    ring->type = ONE_RING_ACTOR_TYPE;
    ring->tickCallback = actor_oneRing_tick;
    ring->triggerCallbacks[0] = NULL;
    ring->as.combat.venomDuration = ONE_RING_DAMAGE;
    PLAYER(playerIndex).stats[STAT_DAMAGE_SLASH] += ONE_RING_DAMAGE;
    ring->actionStateTimer = legacyScale(OneRingTicks, legacyFx_castingLevel(playerActor));
    ring->flags.p = ONE_RING_ACTOR_FLAGS;
    ring->parentActor = playerActor;
    playerActor->flags.p |= ONE_RING_UNSEEN_FLAG;
    playerActor->as.combat.actionStateFlags |= ONE_RING_STATE_FLAG;
    if (playerIndex == ACTIVE_PLAYER_INDEX)
    {
        transition_setEffect(ONE_RING_TRANSITION);
    }
    else
    {
        playerActor->spriteFlags |= ONE_RING_PARTNER_SPRITE_FLAG;
    }
    ring->actionState = ONE_RING_ON;
    PLAYER(playerIndex).ringActor = ring;
    player_addCorruption(playerIndex, ONE_RING_CORRUPTION);
}

// Snare (Two Towers 0x0803BD48, 0x0801E5A4, the trap 0x080178CC, its tick 0x080179F0 and trigger
// 0x080112E0, the shrapnel 0x0803F4CC and hit 0x080116A4): for 9 Spirit, while Frodo has fewer than
// level / 2 traps out (rounded up), he sets one where he stands. It opens to its 4th frame and waits
// until a foe touches it, then bursts into shrapnel in 4 directions (8 from level 10) at speed 5 for 21
// ticks, each hitting the first foe it touches for 8 + 1 per level.
#define SNARE_SKILL 10
#define SNARE_ANIMATION 0x080500A8u
#define SNARE_ARMED_FRAME 3
#define SNARE_ALL_WAYS_LEVEL 10
#define SNARE_HOLD 30
// a trap's phase
#define SNARE_ARMING 0
#define SNARE_SPRUNG 1
#define SNARE_BURSTING 2

static const LegacyScale SnareDamage = {8, 1};

static const u32 ShrapnelSprites[8] = {
    0x0832EBD4u, 0x0832EBDCu, 0x0832EBE4u, 0x0832EBECu,
    0x0832EBF4u, 0x0832EBECu, 0x0832EBE4u, 0x0832EBDCu,
};

static const LegacyProjectileDef Shrapnel = {
    .sprites = ShrapnelSprites,
    .speed = 0x50000,
    .lifetime = 21,
    .sound = LEGACY_SOUND_NONE,
};

static void snareTick(Actor *fx);

static u32 trapsOut(u8 playerIndex)
{
    Actor *actor;
    u32 count = 0;

    for (actor = g_ActorListMain.head; actor != NULL; actor = actor->next)
    {
        if (actor->tickCallback == snareTick && !(actor->flags.p & ACTOR_FLAG_PENDING_REMOVE) &&
            actor->parentActor->as.combat.playerIndex == playerIndex)
        {
            count++;
        }
    }
    return count;
}

static bool32 snareCanCast(u8 playerIndex)
{
    return trapsOut(playerIndex) < (legacySkills_level(SNARE_SKILL) + 1) / 2;
}

static void snareTouch(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other) && LEGACY_FX(fx)->phase == SNARE_ARMING)
    {
        LEGACY_FX(fx)->phase = SNARE_SPRUNG;
        fx->flags.p |= ACTOR_FLAG_ANIMATION_PLAYING;
        fx->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
    }
}

static void snareTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    u8 level = state->count;
    Direction8 facing;

    if (state->phase == SNARE_ARMING && fx->previousAnimationFrame == SNARE_ARMED_FRAME)
    {
        sprite_setAnimationFrame(fx, SNARE_ARMED_FRAME);
        fx->flags.p &= ~ACTOR_FLAG_ANIMATION_PLAYING;
    }
    else if (state->phase == SNARE_SPRUNG)
    {
        state->phase = SNARE_BURSTING;
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
        for (facing = 0; facing < 8; facing += level < SNARE_ALL_WAYS_LEVEL ? 2 : 1)
        {
            legacyFx_castProjectileAt(fx->parentActor, &Shrapnel, fx->coords, facing, state->amount);
        }
    }
    fx->actionStateTimer = state->phase == SNARE_BURSTING && legacyFx_animationEnded(fx) ? 0 : SNARE_HOLD;
    legacyFx_finishTick(fx);
}

static void castSnare(Actor *playerActor)
{
    Actor *fx = legacyFx_spawn(playerActor, SNARE_ANIMATION);
    Vector2Fp16 at = playerActor->coords;
    u8 level = legacyFx_castingLevel(playerActor);

    if (fx == NULL)
    {
        return;
    }
    at.y += 1 << 16;
    legacyFx_place(fx, at);
    fx->flags.p &= ~ACTOR_FLAG_TILE_COLLISION;
    fx->flags.p |= ACTOR_FLAG_COLLISION_CHECK;
    fx->tickCallback = snareTick;
    fx->triggerCallbacks[0] = snareTouch;
    fx->actionStateTimer = SNARE_HOLD;
    LEGACY_FX(fx)->amount = legacyScale(SnareDamage, level);
    LEGACY_FX(fx)->count = level;
}

// ROTK's rows the actives take their cast poses from
static const SkillExpansionActiveDef KnifeRow = {.baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS};
// Knife Toss's pose without its weapon rule, for the casts that are not a throw. (Galadriel's
// Cloak's pose waits for the cloak to end it, so a cast in it never ends.)
static const SkillExpansionCastPolicy PosePolicy = {
    ACTIVE_SKILL_FRODO_KNIFE_TOSS + 1, ACTIVE_SKILL_FRODO_KNIFE_TOSS, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionActiveDef PoseRow = {
    .baseRow = ACTIVE_SKILL_FRODO_KNIFE_TOSS,
    .policy = &PosePolicy,
};
static bool32 oneRingCanCast(u8 playerIndex)
{
    return PLAYER(playerIndex).ringActor == NULL;
}

static const SkillExpansionActiveDef TheOneRingRow = {
    .baseRow = ACTIVE_SKILL_FRODO_THE_ONE_RING,
    .canCast = oneRingCanCast,
};
// Snare's pose, without ROTK's snare limit (it reads its own row's level, 0 in a legacy cast)
static const SkillExpansionCastPolicy SnarePolicy = {
    ACTIVE_SKILL_FRODO_SNARE + 1, ACTIVE_SKILL_FRODO_SNARE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionActiveDef SnareRow = {
    .baseRow = ACTIVE_SKILL_FRODO_SNARE,
    .policy = &SnarePolicy,
    .canCast = snareCanCast,
};

const LegacyActive LegacyFrodoActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [0] = {&TheOneRingRow, 10, castTheOneRing},
    [1] = {&PoseRow, 3, castEscape},
    [2] = {&PoseRow, 4, castCharge},
    [3] = {&PoseRow, 8, castHobbitAppetite, 2},
    [4] = {&KnifeRow, 2, castKnifeThrow},
    [5] = {&SnareRow, 9, castSnare},
};

#endif
