// Gandalf's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"
#include "patches/legacy_skills/legacy_skills.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "actor/shieldBubble.h"
#include "gfx.h"
#include "player.h"
#include "rng.h"
#include "combat.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

// Zap (Two Towers 0x08018FD4, its tick 0x080191F0, hit 0x080115E0): a bolt that flies for 30
// frames and bursts on the first foe it hits for 5 + 2 per level.
static const u32 ZapSprites[8] = {
    0x08050138u, 0x08050148u, 0x08050148u, 0x08050148u,
    0x08050158u, 0x08050158u, 0x08050168u, 0x08050168u,
};

static const LegacyProjectileDef Zap = {
    .sprites = ZapSprites,
    .impact = 0x080501C8u,
    .speed = 0x50000,
    .lifetime = 30,
    .damage = {5, 2},
    .sound = LEGACY_SOUND_BOLT,
};

static void castZap(Actor *playerActor)
{
    legacyFx_castProjectile(playerActor, &Zap);
}

// Fireworks (Two Towers 0x08018280, its tick 0x08018494, hits 0x080114FC and 0x08011728): a rocket
// that flies for 30 ticks and explodes on the first foe it hits, dealing 5 + 2 per level to every
// foe in the blast on the frame it bursts. It is gone without a blast when it runs out. The
// description says 10 + 2 per level; the game deals 5 + 2.
static const u32 FireworksSprites[8] = {
    0x08328208u, 0x08328210u, 0x08328218u, 0x08328220u,
    0x08328228u, 0x08328220u, 0x08328218u, 0x08328210u,
};

static const LegacyProjectileDef Fireworks = {
    .sprites = FireworksSprites,
    .blast = 0x08328230u,
    .speed = 0x50000,
    .lifetime = 30,
    .damage = {5, 2},
    .sound = LEGACY_SOUND_BOLT,
    .blastSound = SFX_COMBAT_FIREBURST_PROC,
};

static void castFireworks(Actor *playerActor)
{
    legacyFx_castProjectile(playerActor, &Fireworks);
}

// Sunburst (Two Towers 0x0803BA44, 0x08018880, its tick 0x08018AC4, hit 0x08011484): a burst at
// Gandalf that stuns the foes within 75 pixels for 30 + 10 per level ticks, for 8 Spirit. It deals
// no damage when cast (its damage is Flame of Anor's burst). Not yet: the 8 rotated rays it sends
// out, which only show.
static const LegacyBurstDef Sunburst = {
    .animation = 0x080500F8u,
    .radius = 75,
    .lifetime = 30,
    .stunTicks = {30, 10},
    .sound = SFX_COMBAT_SUNBURST,
};

static void castSunburst(Actor *playerActor)
{
    legacyFx_castBurst(playerActor, &Sunburst);
}

// Magical Healing (Two Towers 0x0803C4F8, 0x0801EA40): heals 20 + 4 per level for 8 + 2 per level
// Spirit, with a sparkle on Gandalf.
static const LegacyHealDef MagicalHealing = {
    .hp = {20, 4},
    .sparkle = 0x080501D8u,
    .sound = SFX_HERB_PICKUP,
};

static void castMagicalHealing(Actor *playerActor)
{
    legacyFx_castHeal(playerActor, &MagicalHealing);
}

// Shield (Two Towers 0x0803BB1C, 0x080185B4, its tick 0x0801871C): a bubble on Gandalf that absorbs
// 50 + 20 per level damage for 750 frames, for 10 Spirit and 2 corruption per level. One at a time.
// It absorbs through ROTK's own shield bubble (PLAYER().shieldBubble).
#define SHIELD_ABSORB_BASE 50
#define SHIELD_ABSORB_PER_LEVEL 20
#define SHIELD_FRAMES 750
#define SHIELD_CORRUPTION_PER_LEVEL 2
#define SHIELD_FORMING 0x080500C8u
#define SHIELD_UP 0x080500D8u
#define SHIELD_DISSOLVING 0x080500E8u
// the shield's phase, kept in its charge level (Gimli's shields only use that)
#define SHIELD_PHASE_FORMING 0
#define SHIELD_PHASE_UP 1
#define SHIELD_PHASE_DISSOLVING 2

static bool32 shieldCanCast(u8 playerIndex)
{
    return PLAYER(playerIndex).shieldBubble == NULL;
}

static void shieldTick(Actor *fx)
{
    Actor *playerActor = fx->parentActor;
    u8 playerIndex = playerActor->as.combat.playerIndex;
    s16 *phase = &fx->as.shieldGandalf.chargeLevel;
    bool32 ended = legacyFx_animationEnded(fx);

    fx->coords = playerActor->coords;
    // an absorbed hit flashes the bubble in ROTK; it just stays up here
    if (fx->actionState == ACTOR_STATE_SHIELD_SPENT)
    {
        fx->actionState = ACTOR_STATE_FX_ATTACHED;
    }
    if (*phase == SHIELD_PHASE_FORMING && ended)
    {
        *phase = SHIELD_PHASE_UP;
        fx->actionState = ACTOR_STATE_FX_ATTACHED;
        legacyFx_setSprite(fx, SHIELD_UP);
    }
    else if (*phase == SHIELD_PHASE_UP &&
             (fx->as.shieldGandalf.shieldHealth <= 0 || --fx->actionStateTimer < 0))
    {
        *phase = SHIELD_PHASE_DISSOLVING;
        legacyFx_setSprite(fx, SHIELD_DISSOLVING);
    }
    else if (*phase == SHIELD_PHASE_DISSOLVING && ended)
    {
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    if (playerActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        fx->actionState = ACTOR_STATE_UNKNOWN_04;
    }
    if (fx->actionState == ACTOR_STATE_UNKNOWN_04)
    {
        if (PLAYER(playerIndex).shieldBubble == fx)
        {
            PLAYER(playerIndex).shieldBubble = NULL;
        }
        fx->actionStateTimer = 0;
        actor_unknown08045584(fx);
    }
}

static void castShield(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    u8 level = legacyFx_castingLevel(playerActor);
    Actor *fx;

    player_addCorruption(playerIndex, SHIELD_CORRUPTION_PER_LEVEL * level);
    fx = legacyFx_spawn(playerActor, SHIELD_FORMING);
    if (fx == NULL)
    {
        return;
    }
    fx->tickCallback = shieldTick;
    fx->actionStateTimer = SHIELD_FRAMES;
    fx->as.shieldGandalf.followTarget = playerActor;
    fx->as.shieldGandalf.shieldHealth = SHIELD_ABSORB_BASE + SHIELD_ABSORB_PER_LEVEL * level;
    fx->as.shieldGandalf.chargeLevel = SHIELD_PHASE_FORMING;
    PLAYER(playerIndex).shieldBubble = fx;
    sfx_play(SFX_COMBAT_GANDALF_SHIELD_CAST);
}

// Thunderstrike (Two Towers 0x0801E658; the storm 0x08018D7C, its tick 0x08018EC0 and its pick
// 0x08011440; the bolt 0x08027A24 and its tick 0x08027B44): a storm around Gandalf strikes a foe
// within 80 pixels every 4 ticks, 5 + level / 3 times, for 20 + 2 per level each, and ends when no
// foe is in reach. Among several foes in reach each new one replaces the pick on an even roll. A
// strike is a bolt that falls on the foe and hits when its animation ends. The storm is unseen. The
// storm keeps its strikes left in its count and the ticks to the next strike in its phase.
#define THUNDERSTRIKE_STRIKES_BASE 5
#define THUNDERSTRIKE_LEVELS_PER_STRIKE 3
#define THUNDERSTRIKE_DAMAGE_BASE 20
#define THUNDERSTRIKE_DAMAGE_PER_LEVEL 2
#define THUNDERSTRIKE_RADIUS 80
#define THUNDERSTRIKE_PAUSE 4
#define THUNDERSTRIKE_LIFETIME 15
#define THUNDERSTRIKE_BOLT 0x08052690u

static void boltTick(Actor *fx)
{
    Actor *foe = LEGACY_FX(fx)->target;

    if (foe != NULL)
    {
        fx->coords = foe->coords;
    }
    if (legacyFx_animationEnded(fx))
    {
        if (foe != NULL && !(foe->flags.p & ACTOR_FLAG_PENDING_REMOVE))
        {
            combat_applyPveRangedHit(fx->parentActor, foe, LEGACY_FX(fx)->amount);
        }
        LEGACY_FX(fx)->target = NULL;
        fx->actionStateTimer = 0;
    }
    legacyFx_finishTick(fx);
}

static void spawnBolt(Actor *storm, Actor *foe)
{
    Actor *fx = legacyFx_spawn(storm->parentActor, THUNDERSTRIKE_BOLT);

    if (fx == NULL)
    {
        return;
    }
    fx->coords = foe->coords;
    fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    LEGACY_FX(fx)->target = foe;
    LEGACY_FX(fx)->amount = LEGACY_FX(storm)->amount;
    fx->actionStateTimer = THUNDERSTRIKE_LIFETIME;
    fx->tickCallback = boltTick;
}

static void stormPick(Actor *fx, Actor *other)
{
    if (!legacyFx_isFoe(other))
    {
        return;
    }
    if (LEGACY_FX(fx)->target == NULL || rng_rollUpTo(100) <= rng_rollUpTo(100))
    {
        LEGACY_FX(fx)->target = other;
    }
}

static void stormTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    u8 *pause = &state->phase;

    fx->coords = fx->parentActor->coords;
    if (state->count == 0)
    {
        legacyFx_end(fx);
    }
    else
    {
        if (state->target != NULL)
        {
            spawnBolt(fx, state->target);
            sfx_play(LEGACY_SOUND_BOLT);
            state->target = NULL;
            state->count--;
            *pause = THUNDERSTRIKE_PAUSE;
        }
        // it looks for a foe on the tick the pause runs out, and ends when it finds none
        if (*pause == 0)
        {
            legacyFx_end(fx);
        }
        else if (--*pause == 0)
        {
            fx->flags.p |= ACTOR_FLAG_COLLISION_CHECK;
        }
        else
        {
            fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
        }
    }
    if (fx->parentActor->actionState == ACTOR_STATE_UNKNOWN_2D)
    {
        legacyFx_end(fx);
    }
    if (fx->actionState != ACTOR_STATE_UNKNOWN_04)
    {
        fx->actionStateTimer = THUNDERSTRIKE_LIFETIME;
    }
    actor_unknown08045584(fx);
}

static void castThunderstrike(Actor *playerActor)
{
    u8 level = legacyFx_castingLevel(playerActor);
    Actor *fx = legacyFx_spawn(playerActor, 0);

    if (fx == NULL)
    {
        return;
    }
    fx->flags.p &= ~ACTOR_FLAG_RENDER;
    LEGACY_FX(fx)->target = NULL;
    fx->tickCallback = stormTick;
    fx->triggerCallbacks[0] = stormPick;
    fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY | ACTOR_FLAG_COLLISION_CHECK;
    fx->interactionRadius = THUNDERSTRIKE_RADIUS;
    LEGACY_FX(fx)->amount = THUNDERSTRIKE_DAMAGE_BASE + THUNDERSTRIKE_DAMAGE_PER_LEVEL * level;
    LEGACY_FX(fx)->count = THUNDERSTRIKE_STRIKES_BASE + level / THUNDERSTRIKE_LEVELS_PER_STRIKE;
    LEGACY_FX(fx)->phase = 1;
    fx->actionStateTimer = THUNDERSTRIKE_LIFETIME;
    sfx_play(LEGACY_SOUND_BOLT);
}

// Summon Eagle (Two Towers 0x0803BC04; the eagle 0x08017AD4, its tick 0x08017C64 and its pick
// 0x0801151C): for 12 Spirit and 3 corruption per level, one eagle at a time that swoops once per
// level for 20 + 5 per level. Each swoop it waits unseen, up to 16 ticks, for a foe within 75 pixels
// of where it waits (where Gandalf cast it, then where he is when it comes back). It comes in from
// 160 pixels left or right of Gandalf, the sides alternating, at a random height within 60 of him,
// flies at the foe, strikes it and flies off the screen sideways. It is gone when no foe comes, its
// swoops are spent or Gandalf is down.
static const LegacyScale EagleDamage = {20, 5};
#define EAGLE_CORRUPTION_PER_LEVEL 3
#define EAGLE_WAIT 16
#define EAGLE_HOLD 30
#define EAGLE_RADIUS 75
#define EAGLE_SPEED 0x50000
#define EAGLE_ENTRY_X (160 << 16)
#define EAGLE_ENTRY_Y 60
#define EAGLE_FLY 0x083281A0u
#define EAGLE_STRIKE 0x08328190u
#define EAGLE_LEAVE 0x08328198u
// how far past each screen edge the eagle still counts as on screen, in pixels
#define EAGLE_SCREEN_MARGIN 40
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
// actor_direction8TowardPoint: within the tolerance on both axes
#define DIRECTION8_ARRIVED 8
// the eagle's phase, in LegacyFxState.phase with the side it comes from next
#define EAGLE_WAITING 0
#define EAGLE_SWOOPING 1
#define EAGLE_STRIKING 2
#define EAGLE_LEAVING 3
#define EAGLE_PHASE_MASK 3
#define EAGLE_FROM_LEFT 0x80

// Each player's eagle, while it is out.
static Actor *eagles[2];

static void eagleTick(Actor *fx);

static bool32 eagleOut(u8 playerIndex)
{
    Actor *eagle = eagles[playerIndex];

    return eagle != NULL && eagle->tickCallback == eagleTick && !(eagle->flags.p & ACTOR_FLAG_PENDING_REMOVE);
}

static bool32 eagleCanCast(u8 playerIndex)
{
    return !eagleOut(playerIndex);
}

static bool32 eagleOnScreen(Actor *fx)
{
    Vector2Int camera;
    s32 x = fx->coords.x >> 16, y = fx->coords.y >> 16;

    gfx_getCameraPosition(&camera, fx->parentActor->as.combat.playerIndex);
    return x >= camera.x - EAGLE_SCREEN_MARGIN && x <= camera.x + SCREEN_WIDTH + EAGLE_SCREEN_MARGIN &&
           y >= camera.y - EAGLE_SCREEN_MARGIN && y <= camera.y + SCREEN_HEIGHT + EAGLE_SCREEN_MARGIN;
}

static void eagleHead(Actor *fx, Direction8 facing)
{
    fx->facing = facing;
    actor_setVelocityFromDirection8(&fx->velocity, fx->flags.p, facing, EAGLE_SPEED);
}

// it waits unseen where Gandalf is, looking for a foe
static void eagleWait(Actor *fx)
{
    LEGACY_FX(fx)->phase = (LEGACY_FX(fx)->phase & ~EAGLE_PHASE_MASK) | EAGLE_WAITING;
    LEGACY_FX(fx)->target = NULL;
    fx->velocity.x = 0;
    fx->velocity.y = 0;
    fx->coords = fx->parentActor->coords;
    fx->flags.p &= ~(ACTOR_FLAG_RENDER | ACTOR_FLAG_ANIMATION_PLAYING);
    fx->flags.p |= ACTOR_FLAG_COLLISION_CHECK;
    fx->actionStateTimer = EAGLE_WAIT;
}

static void eagleSwoop(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    Actor *foe = state->target;
    Vector2Fp16 from = fx->parentActor->coords;

    // it comes from the side it did not come from last
    if (state->phase & EAGLE_FROM_LEFT)
    {
        from.x -= EAGLE_ENTRY_X;
        state->phase &= ~EAGLE_FROM_LEFT;
    }
    else
    {
        from.x += EAGLE_ENTRY_X;
        state->phase |= EAGLE_FROM_LEFT;
    }
    from.y += rng_rollRange(-EAGLE_ENTRY_Y, EAGLE_ENTRY_Y) << 16;
    fx->coords = from;
    state->phase = (state->phase & ~EAGLE_PHASE_MASK) | EAGLE_SWOOPING;
    fx->flags.p |= ACTOR_FLAG_RENDER | ACTOR_FLAG_ANIMATION_PLAYING;
    legacyFx_setSprite(fx, EAGLE_FLY);
    eagleHead(fx, actor_direction8TowardPoint(from.x, from.y, foe->coords.x, foe->coords.y, EAGLE_SPEED));
}

static void eaglePick(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other) && (LEGACY_FX(fx)->phase & EAGLE_PHASE_MASK) == EAGLE_WAITING)
    {
        LEGACY_FX(fx)->target = other;
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
}

static void eagleTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    Actor *foe = state->target;
    u32 phase = state->phase & EAGLE_PHASE_MASK;


    if (phase == EAGLE_WAITING)
    {
        if (foe != NULL)
        {
            eagleSwoop(fx);
        }
    }
    else if (phase == EAGLE_SWOOPING)
    {
        if (foe->flags.p & ACTOR_FLAG_PENDING_REMOVE)
        {
            state->phase = (state->phase & ~EAGLE_PHASE_MASK) | EAGLE_LEAVING;
        }
        else
        {
            Direction8 facing = actor_direction8TowardPoint(fx->coords.x, fx->coords.y, foe->coords.x,
                                                            foe->coords.y, EAGLE_SPEED);

            if (facing == DIRECTION8_ARRIVED)
            {
                fx->velocity.x = 0;
                fx->velocity.y = 0;
                state->phase = (state->phase & ~EAGLE_PHASE_MASK) | EAGLE_STRIKING;
                legacyFx_setSprite(fx, EAGLE_STRIKE);
                combat_applyPveRangedHit(fx->parentActor, foe, state->amount);
            }
            else
            {
                eagleHead(fx, facing);
            }
        }
    }
    else if (phase == EAGLE_STRIKING)
    {
        if (legacyFx_animationEnded(fx))
        {
            state->phase = (state->phase & ~EAGLE_PHASE_MASK) | EAGLE_LEAVING;
            legacyFx_setSprite(fx, EAGLE_LEAVE);
            // on across the screen, away from the side it came from
            eagleHead(fx, (state->phase & EAGLE_FROM_LEFT) ? DIRECTION8_WEST : DIRECTION8_EAST);
        }
    }
    else if (!eagleOnScreen(fx))
    {
        if (--state->count == 0)
        {
            legacyFx_end(fx);
        }
        else
        {
            eagleWait(fx);
        }
    }
    if ((state->phase & EAGLE_PHASE_MASK) != EAGLE_WAITING && fx->actionState != ACTOR_STATE_UNKNOWN_04)
    {
        fx->actionStateTimer = EAGLE_HOLD;
    }
    legacyFx_faceSprite(fx);
    legacyFx_finishTick(fx);
}

static void castSummonEagle(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    u8 level = legacyFx_castingLevel(playerActor);
    Actor *fx;

    player_addCorruption(playerIndex, EAGLE_CORRUPTION_PER_LEVEL * level);
    fx = legacyFx_spawn(playerActor, 0);
    if (fx == NULL)
    {
        return;
    }
    fx->tickCallback = eagleTick;
    fx->triggerCallbacks[0] = eaglePick;
    // it flies over walls and past the map's edges
    fx->flags.p &= ~ACTOR_FLAG_TILE_COLLISION;
    fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY;
    fx->interactionRadius = EAGLE_RADIUS;
    LEGACY_FX(fx)->amount = legacyScale(EagleDamage, level);
    LEGACY_FX(fx)->count = level;
    LEGACY_FX(fx)->phase = rng_rollUpTo(1) ? EAGLE_FROM_LEFT : 0;
    eagleWait(fx);
    eagles[playerIndex] = fx;
}

// Flame of Anor (Two Towers: the revive in player_tick at 0x0801BB2A, then 0x08018880 not cast):
// when Gandalf falls, its revive chance (the passive) brings him back at full HP, not ROTK's half,
// with a Sunburst that stuns every foe within 75 pixels for 30 + 10 per level and deals them 5 per
// level.
#define FLAME_OF_ANOR_SKILL 3

static const LegacyBurstDef FlameOfAnorBurst = {
    .animation = 0x080500F8u,
    .radius = 75,
    .lifetime = 30,
    .stunTicks = {30, 10},
    .damage = {0, 5},
    .sound = SFX_COMBAT_SUNBURST,
};

void legacySkills_revive(u8 playerIndex)
{
    u8 level = legacySkills_loaded(playerIndex) && PLAYER(playerIndex).heroId == HERO_ID_GANDALF
                   ? legacySkills_level(FLAME_OF_ANOR_SKILL)
                   : 0;

    if (level == 0)
    {
        player_addHp(playerIndex, PLAYER_STAT(playerIndex, STAT_MAX_HP) / 2, FALSE);
        return;
    }
    player_addHp(playerIndex, PLAYER_STAT(playerIndex, STAT_MAX_HP), FALSE);
    legacyFx_castBurstAt(PLAYER(playerIndex).ownerActor, &FlameOfAnorBurst, level);
}

// legacy_skills.asm jumps here from the stock revive (actor_player_tick at 0x08023FDA), in place of
// its player_addHp(playerIndex, maxHp / 2): r10 holds the player index and r4 the Player, which the
// stock code leaves pointing at its max HP.
asm(".section .text\n.balign 4\n.thumb\n.global legacySkills_reviveHook\n.thumb_func\n"
    "legacySkills_reviveHook:\n"
    " add r4, #0xd0\n"
    " mov r0, r10\n"
    " bl legacySkills_revive\n"
    " ldr r3, =0x08023FEF\n"
    " bx r3\n"
    " .pool\n");

// a port: Lightstrike's cast pose and requirements, its own cast
static const SkillExpansionActiveDef Lightstrike = {.baseRow = ACTIVE_SKILL_GANDALF_LIGHTSTRIKE};
static const SkillExpansionActiveDef ShieldRow = {
    .baseRow = ACTIVE_SKILL_GANDALF_SHIELD,
    .canCast = shieldCanCast,
};
static const SkillExpansionActiveDef BlindingAura = {.baseRow = ACTIVE_SKILL_GANDALF_BLINDING_AURA};
// Shield's pose: casting through Herbal Healing's row hangs the game
static const SkillExpansionActiveDef HealingRow = {.baseRow = ACTIVE_SKILL_GANDALF_SHIELD};
// Summon Gwaihir's cast pose, one eagle at a time
static const SkillExpansionActiveDef EagleRow = {
    .baseRow = ACTIVE_SKILL_GANDALF_SUMMON_GWAIHIR,
    .canCast = eagleCanCast,
};

const LegacyActive LegacyGandalfActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    {&HealingRow, 8, castMagicalHealing, 2},
    {&ShieldRow, 10, castShield},
    {&Lightstrike, 3, castZap},
    {&BlindingAura, 8, castSunburst},
    {&Lightstrike, 5, castFireworks},
    {&EagleRow, 12, castSummonEagle},
    {&BlindingAura, 20, castThunderstrike},
};

#endif
