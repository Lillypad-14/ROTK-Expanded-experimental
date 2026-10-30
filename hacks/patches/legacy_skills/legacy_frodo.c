// Frodo's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "combat.h"
#include "lib/movement.h"
#include "sfx.h"
#include "stats.h"

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
// stand-ins: ROTK's The One Ring and Snare
static void theOneRing(Actor *playerActor)
{
    skill_active_frodoTheOneRing(playerActor);
}

static void snare(Actor *playerActor)
{
    skill_active_frodoSnare(playerActor);
}

static const SkillExpansionActiveDef TheOneRing = {
    .stockCallback = theOneRing,
    .baseRow = ACTIVE_SKILL_FRODO_THE_ONE_RING,
};
// ROTK's snare limit reads its own row's level, which a legacy cast leaves at 0
static const SkillExpansionCastPolicy SnarePolicy = {
    ACTIVE_SKILL_FRODO_SNARE + 1, ACTIVE_SKILL_FRODO_SNARE, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionActiveDef Snare = {
    .stockCallback = snare,
    .baseRow = ACTIVE_SKILL_FRODO_SNARE,
    .policy = &SnarePolicy,
};

const LegacyActive LegacyFrodoActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [0] = {&TheOneRing, 10},
    [1] = {&PoseRow, 3, castEscape},
    [2] = {&PoseRow, 4, castCharge},
    [3] = {&PoseRow, 8, castHobbitAppetite, 2},
    [4] = {&KnifeRow, 2, castKnifeThrow},
    [5] = {&Snare, 9},
};

#endif
