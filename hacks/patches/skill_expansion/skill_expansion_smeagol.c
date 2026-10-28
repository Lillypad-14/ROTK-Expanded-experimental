#include "patches/skill_expansion/skill_expansion_smeagol.h"
#include "fx.h"
#include "game.h"
#include "gba_io.h"
#include "input.h"
#include "lib/actor_ref.h"
#include "lib/fx_kit.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/targeting.h"
#include "lib/timer.h"
#include "loot.h"
#include "math.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_gandalf.h"
#include "patches/skill_expansion/skill_expansion_legolas.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "rng.h"
#include "scene.h"
#include "sfx.h"
#include "stats.h"
#include "variables.h"

#define FRAMES_PER_SECOND 30
#define GRIP_MAX 100

// Cast visuals step every CAST_FX_TICKS so they read instead of flashing by.
#define CAST_FX_TICKS 3

// Gollum's Grip
#define GRIP_COMBAT_RADIUS 112
#define GRIP_CHECK_FRAMES 15
#define GRIP_GAIN_FRAMES 18
#define GRIP_FADE_FRAMES 45
#define GRIP_HIT_GAIN 2
#define GRIP_KILL_GAIN 3
#define GRIP_AFTER_TAKEOVER 40
#define BARK_COOLDOWN_FRAMES (4 * FRAMES_PER_SECOND)

// Gollum Takes Over
#define TAKEOVER_FRAMES (4 * FRAMES_PER_SECOND)
#define TAKEOVER_MASH_FRAMES 6
#define TAKEOVER_SEEK_RADIUS 160
#define TAKEOVER_REACH 22
#define TAKEOVER_DEADZONE 4
#define TAKEOVER_ATTACK_FRAMES 9
#define TAKEOVER_DAMAGE_MULTIPLIER 2

// Play Dead (Smeagol)
#define PLAY_DEAD_BASE_FRAMES (2 * FRAMES_PER_SECOND)
#define PLAY_DEAD_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define PLAY_DEAD_GRIP 10
#define PLAY_DEAD_STRIKE_FRAMES (3 * FRAMES_PER_SECOND)
#define PLAY_DEAD_STRIKE_MULTIPLIER 3

// Catch a Fish (Smeagol)
#define CATCH_BASE_FRAMES (3 * FRAMES_PER_SECOND / 2)
#define CATCH_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 4)
#define CATCH_RADIUS 36
#define CATCH_BASE_DAMAGE 10
#define CATCH_DAMAGE_PER_LEVEL 4
#define CATCH_DAMAGE_MULTIPLIER 2
#define CATCH_GRIP 3
#define CATCH_AIM_RADIUS 160

// Throttle (Gollum)
#define THROTTLE_REACH 28
#define THROTTLE_BASE_FRAMES (3 * FRAMES_PER_SECOND)
#define THROTTLE_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define THROTTLE_STEP 2
#define THROTTLE_GRIP 10
#define THROTTLE_CHOKE_FRAMES FRAMES_PER_SECOND
#define THROTTLE_CHOKE_BASE_DAMAGE 3
#define THROTTLE_CHOKE_DAMAGE_PER_LEVEL 2
#define THROTTLE_RAM_FRAMES 8
#define THROTTLE_RAM_RADIUS 20
#define THROTTLE_RAM_BASE_DAMAGE 6
#define THROTTLE_RAM_DAMAGE_PER_LEVEL 3
#define THROTTLE_RAM_STUN_FRAMES FRAMES_PER_SECOND

// Slink (Gollum)
#define SLINK_MAX_DISTANCE 56
#define SLINK_MIN_DISTANCE 16
#define SLINK_DISTANCE_STEP 8
#define SLINK_BASE_HIDE_FRAMES FRAMES_PER_SECOND
#define SLINK_HIDE_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 4)
#define SLINK_GRIP 5

// Drop the Precious (Precious)
#define BAIT_FRAMES (8 * FRAMES_PER_SECOND)
#define BAIT_ARM_FRAMES (FRAMES_PER_SECOND / 3)
#define BAIT_PICKUP_RADIUS 14
#define BAIT_LEAVE_RADIUS 32
#define BAIT_LURE_BASE_RADIUS 96
#define BAIT_LURE_RADIUS_PER_LEVEL 8
#define BAIT_LURE_FRAMES 3
#define BAIT_SURGE_RADIUS 56
#define BAIT_SURGE_BASE_DAMAGE 15
#define BAIT_SURGE_DAMAGE_PER_LEVEL 6
#define BAIT_SURGE_STUN_FRAMES (2 * FRAMES_PER_SECOND)
#define BAIT_SURGE_GRIP 20

// Argue With Yourself (Precious)
#define ARGUE_BASE_GRIP 25
#define ARGUE_GRIP_PER_LEVEL 5
#define ARGUE_SHOUT_GRIP 60
#define ARGUE_SHOUT_RADIUS 48
#define ARGUE_SHOUT_BASE_STUN_FRAMES FRAMES_PER_SECOND
#define ARGUE_SHOUT_STUN_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 4)

// Passives
#define POOR_SMEAGOL_FRAMES_PER_LEVEL 3
#define HUNGRY_PERCENT_PER_LEVEL 1 // per 10 grip
#define SNIFF_FRAMES (2 * FRAMES_PER_SECOND)
#define SNIFF_BASE_RADIUS 80
#define SNIFF_RADIUS_PER_LEVEL 16
#define NICE_MASTER_HEAL_PERCENT_PER_LEVEL 4
#define RAT_CATCHER_GRIP 10
#define RAT_CATCHER_SPIRIT_PER_LEVEL 4
#define FINDERS_KEEPERS_PERCENT_PER_LEVEL 15
#define FREE_FRAMES (5 * FRAMES_PER_SECOND)
#define WE_WANTS_IT_FRAMES (2 * FRAMES_PER_SECOND)
#define WE_WANTS_IT_KILL_FRAMES FRAMES_PER_SECOND
#define WE_WANTS_IT_MAX_FRAMES (8 * FRAMES_PER_SECOND)
#define MY_PRECIOUS_GRIP 70

// Gollum's colors fade in over GOLLUM_FADE_STEPS frames, one step per frame.
// actor_applyTypeDefaults loads Smeagol with the palette of hero id 9, not 7.
#define SMEAGOL_OBJ_PALETTE (0x21 + 9)
#define GOLLUM_FADE_STEPS FX_TINT_FULL
#define TINT_REFRESH_FRAMES FRAMES_PER_SECOND
#define TAKEOVER_SPARK_FRAMES (FRAMES_PER_SECOND / 2)

// Actor type of a stock arrow, bolt or thrown rock in flight. parentActor is the shooter.
#define PROJECTILE_ACTOR_TYPE 0x37

// Set on the player by Galadriel's Cloak and the One Ring while enemies can't see them.
#define ACTOR_FLAG_HIDDEN ACTOR_FLAG_23

typedef struct SmeagolState
{
    // Gollum Takes Over frames left
    u16 takeoverFrames;

    // takeover frames A presses cut this takeover
    u16 mashedFrames;

    // A presses since the last tick during a takeover
    u8 mashPresses;

    // Gollum's Grip, 0..GRIP_MAX
    u8 grip;

    // highest bark threshold announced since the grip last fell below it
    u8 barkBand;

    // frames before another grip bark can show
    u8 barkFrames;

    // frames until the grip next rises (in combat) or fades (out of combat)
    u8 gripFrames;

    // frames until the next foe proximity check
    u8 combatCheckFrames;

    // TRUE while a foe is within GRIP_COMBAT_RADIUS
    bool inCombat;

    // TRUE while Smeagol has ACTOR_FLAG_HIDDEN set
    bool hiding;

    // Play Dead frames left
    u16 playDeadFrames;

    // frames the x3 strike after Play Dead stays ready
    u16 strikeFrames;

    // TRUE while a Play Dead strike resolves
    bool striking;

    // TRUE when B was pressed during Play Dead
    bool getUp;

    // Catch a Fish window frames left
    u16 catchFrames;

    // projectiles Catch a Fish can still catch
    u8 catchesLeft;

    // Catch a Fish level at cast
    u8 catchLevel;

    // row Catch a Fish was cast from
    u8 catchRow;

    // Throttle level at cast
    u8 throttleLevel;

    // row Throttle was cast from
    u8 throttleRow;

    // d-pad keys the player held this frame during Throttle
    u8 steerKeys;

    // foe Throttle holds
    HackLibActorRef throttled;

    // Throttle frames left
    u16 throttleFrames;

    // Slink and bait hide frames left
    u16 hideFrames;

    // Drop the Precious frames left
    u16 baitFrames;

    // frames Smeagol must spend away from the dropped Ring before he can grab it
    u8 baitArmFrames;

    // Drop the Precious level at cast
    u8 baitLevel;

    // row Drop the Precious was cast from
    u8 baitRow;

    // dropped Ring position
    Vector2Fp16 baitPosition;

    // dropped Ring visual
    HackLibActorRef baitFx;

    // Smeagol Is Free invulnerability frames left
    u16 freeFrames;

    // frames until the next Sniff It Out mark
    u8 sniffFrames;

    // frames until the next grip spark
    u8 sparkFrames;

    // frames until the Gollum colors are reloaded
    u8 tintFrames;

    // Gollum colors loaded, 0 (stock Smeagol) to GOLLUM_FADE_STEPS
    u8 tintStrength;
} SmeagolState;

_Static_assert(sizeof(SmeagolState) <= sizeof(skillExpansion_heroState[0]),
               "SmeagolState exceeds the hero state");

static inline SmeagolState *state(u8 playerIndex)
{
    return (SmeagolState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_SMEAGOL,
                                                    PLAYER(playerIndex).heroId);
}

static const Vector2Int FacingVectors[8] = {
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
};

// Barks at grip 25, 50, 75 and 90.
static const u8 BarkThresholds[] = {25, 50, 75, 90};
static const char *const Barks[] = {
    "Hungry... so hungry, precious",
    "We hates them, precious!",
    "Kill it, precious! Kill it!",
    "No! Nice master, nice!",
};

static inline bool isSmeagol(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_SMEAGOL &&
           skillExpansion_playerExpanded(playerIndex);
}

static inline s32 passiveLevel(u8 playerIndex, u8 passiveSkillId)
{
    return skillExpansion_getPassiveLevel(playerIndex, passiveSkillId);
}

static inline u8 castLevel(u8 playerIndex)
{
    return skillExpansion_effectiveActiveLevel(playerIndex, PLAYER(playerIndex).castingActiveSkill);
}

static Actor *castFx(Actor *playerActor, u8 shape, HackLibFxColor color, u8 layer)
{
    return hackLib_fxPace(hackLib_fxBurst(playerActor, shape, color, layer), CAST_FX_TICKS);
}

static bool32 isFoeTarget(Actor *target, u32 argument)
{
    (void)argument;
    return skillExpansion_isFoe(target);
}

static Actor *nearestFoe(Actor *center, u16 radius)
{
    HackLibTargetQuery query = {.center = center, .radius = radius, .filter = isFoeTarget};

    return hackLib_nearestTarget(&query);
}

static s32 distanceSquared(const Vector2Fp16 *a, const Vector2Fp16 *b)
{
    s32 dx = (a->x - b->x) >> FP16_16_SHIFT;
    s32 dy = (a->y - b->y) >> FP16_16_SHIFT;

    return dx * dx + dy * dy;
}

static void moveActor(Actor *actor, const Vector2Fp16 *position)
{
    actor->renderCoords.x += position->x - actor->coords.x;
    actor->renderCoords.y += position->y - actor->coords.y;
    actor->coords = *position;
    actor->velocity.x = actor->velocity.y = 0;
}

static void addGrip(u8 playerIndex, s32 amount)
{
    SmeagolState *st = state(playerIndex);
    s32 grip = st->grip + amount;

    st->grip = grip < 0 ? 0 : grip > GRIP_MAX ? GRIP_MAX : grip;
}

static void bark(u8 playerIndex, const char *message)
{
    skillExpansion_feedback(playerIndex, message);
    state(playerIndex)->barkFrames = BARK_COOLDOWN_FRAMES;
}

/** Keep ACTOR_FLAG_HIDDEN set while Play Dead, Slink or the bait hides Smeagol. */
static void updateHiding(SmeagolState *st, Actor *actor)
{
    bool hide = st->playDeadFrames != 0 || st->hideFrames != 0 || st->baitFrames != 0;

    if (hide)
    {
        actor->flags.p |= ACTOR_FLAG_HIDDEN;
    }
    else if (st->hiding)
    {
        actor->flags.p &= ~ACTOR_FLAG_HIDDEN;
    }
    st->hiding = hide;
}

static Actor *throttledFoe(SmeagolState *st)
{
    Actor *foe = st->throttled.actor;

    if (foe == NULL || !hackLib_isActorRefValid(st->throttled) || foe->currentHp <= 0)
    {
        return NULL;
    }
    return foe;
}

static void endThrottle(SmeagolState *st, Actor *actor)
{
    Vector2Fp16 landing;

    st->throttleFrames = 0;
    st->throttled.actor = NULL;
    if (actor != NULL && hackLib_findLanding(actor, &actor->coords, &landing))
    {
        moveActor(actor, &landing);
    }
}

static void endPlayDead(SmeagolState *st)
{
    st->playDeadFrames = 0;
    st->getUp = FALSE;
    st->strikeFrames = PLAY_DEAD_STRIKE_FRAMES;
}

static void startTakeover(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    st->takeoverFrames =
        TAKEOVER_FRAMES + (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WE_WANTS_IT) != 0
                               ? WE_WANTS_IT_FRAMES
                               : 0);
    st->mashedFrames = 0;
    st->mashPresses = 0;
    st->grip = GRIP_MAX;
    st->sparkFrames = 0;
    if (st->playDeadFrames != 0)
    {
        endPlayDead(st);
    }
    if (st->throttleFrames != 0)
    {
        endThrottle(st, actor);
    }
    bark(playerIndex, "GOLLUM! GOLLUM!");
    skillExpansion_spawnBurstFx(actor, FX_AOE_BURST_NIGHT, SFX_COMBAT_TRIPLE_DAMAGE);
    castFx(actor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GREEN, FX_LAYER_FRONT);
}

static void endTakeover(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    bool beaten = st->mashedFrames * 2 >= TAKEOVER_FRAMES;

    st->takeoverFrames = 0;
    st->grip = GRIP_AFTER_TAKEOVER;
    st->barkBand = 0;
    if (beaten && passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SMEAGOL_IS_FREE) != 0)
    {
        st->freeFrames = FREE_FRAMES;
        player_addSpirit(playerIndex, PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT));
        castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        sfx_play(SFX_COMBAT_SUNBURST);
        bark(playerIndex, "Smeagol is free!");
    }
    else
    {
        hackLib_fxBurst(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_GREEN, FX_LAYER_FRONT);
        bark(playerIndex, beaten ? "Go away! Leave Smeagol!" : "Sorry, master... sorry");
    }
}

static void tickTakeover(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    s32 cut = 1 + st->mashPresses * TAKEOVER_MASH_FRAMES;

    st->mashedFrames += st->mashPresses * TAKEOVER_MASH_FRAMES;
    st->mashPresses = 0;
    if (st->takeoverFrames <= cut)
    {
        endTakeover(playerIndex, st, actor);
        return;
    }
    st->takeoverFrames -= cut;
}

static void announceGrip(u8 playerIndex, SmeagolState *st)
{
    u32 band = 0;

    while (band < ARRAY_COUNT(BarkThresholds) && st->grip >= BarkThresholds[band])
    {
        band++;
    }
    if (band < st->barkBand)
    {
        st->barkBand = band;
    }
    else if (band > st->barkBand && st->barkFrames == 0)
    {
        st->barkBand = band;
        bark(playerIndex, Barks[band - 1]);
    }
}

/** The grip climbs while foes are near and fades out of combat. */
static void tickGrip(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    if (st->combatCheckFrames == 0 || --st->combatCheckFrames == 0)
    {
        bool near = nearestFoe(actor, GRIP_COMBAT_RADIUS) != NULL;

        st->combatCheckFrames = GRIP_CHECK_FRAMES;
        if (near != st->inCombat)
        {
            st->inCombat = near;
            st->gripFrames = 0;
        }
    }
    if (st->gripFrames != 0 && --st->gripFrames != 0)
    {
        return;
    }
    if (st->inCombat)
    {
        addGrip(playerIndex, 1);
        st->gripFrames =
            GRIP_GAIN_FRAMES + POOR_SMEAGOL_FRAMES_PER_LEVEL *
                                   passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_POOR_SMEAGOL);
    }
    else
    {
        addGrip(playerIndex, -1);
        st->gripFrames = GRIP_FADE_FRAMES;
    }
}

static void tickPlayDead(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    (void)playerIndex;
    if (st->strikeFrames != 0)
    {
        st->strikeFrames--;
    }
    if (st->playDeadFrames == 0)
    {
        return;
    }
    actor->velocity.x = actor->velocity.y = 0;
    if (st->getUp || --st->playDeadFrames == 0)
    {
        endPlayDead(st);
        hackLib_fxBurst(actor, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
    }
}

static bool isEnemyProjectile(Actor *projectile)
{
    Actor *shooter = projectile->parentActor;

    return projectile->type == PROJECTILE_ACTOR_TYPE &&
           !(projectile->flags.p & ACTOR_FLAG_PENDING_REMOVE) && shooter != NULL &&
           shooter->type != ACTOR_TYPE_NPC && !skillExpansion_isPlayerAvatar(shooter);
}

/** Snatch enemy projectiles near Smeagol and throw them back. */
static void tickCatch(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    Actor *cursor;

    if (st->catchFrames == 0)
    {
        return;
    }
    for (cursor = g_ActorListMain.head; cursor != NULL && st->catchesLeft != 0;
         cursor = cursor->next)
    {
        Actor *shooter;
        Actor *target;
        s32 damage;

        if (!isEnemyProjectile(cursor) ||
            distanceSquared(&cursor->coords, &actor->coords) > CATCH_RADIUS * CATCH_RADIUS)
        {
            continue;
        }
        shooter = cursor->parentActor;
        hackLib_fxAt(actor, &cursor->coords, FX_SHAPE_SPARK, FX_COLOR_WHITE);
        cursor->flags.p |= ACTOR_FLAG_PENDING_REMOVE;
        target = hackLib_isLiveActor(shooter) && skillExpansion_isFoe(shooter)
                     ? shooter
                     : nearestFoe(actor, CATCH_AIM_RADIUS);
        st->catchesLeft--;
        addGrip(playerIndex, -CATCH_GRIP);
        sfx_play(SFX_COMBAT_BLOCK);
        if (target == NULL)
        {
            continue;
        }
        // The catch is thrown straight back and lands at once.
        damage =
            (CATCH_BASE_DAMAGE + CATCH_DAMAGE_PER_LEVEL * st->catchLevel) * CATCH_DAMAGE_MULTIPLIER;
        skillExpansion_dealDamage(actor, target, damage, HACKLIB_DAMAGE_ABILITY, st->catchRow);
        hackLib_fxBurst(target, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
        skillExpansion_feedback(playerIndex, "Catch a fish, precious!");
    }
    if (st->catchesLeft == 0 || --st->catchFrames == 0)
    {
        st->catchFrames = 0;
    }
}

static void ratCatcher(u8 playerIndex, Actor *actor)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_RAT_CATCHER);

    if (level == 0)
    {
        return;
    }
    addGrip(playerIndex, RAT_CATCHER_GRIP);
    player_addSpirit(playerIndex, RAT_CATCHER_SPIRIT_PER_LEVEL * level);
    hackLib_fxBurst(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GREEN, FX_LAYER_FRONT);
}

static void ramFoes(u8 playerIndex, SmeagolState *st, Actor *actor, Actor *foe)
{
    HackLibTargetQuery query = {
        .center = foe, .exclude = actor, .radius = THROTTLE_RAM_RADIUS, .filter = isFoeTarget};
    Actor *other = NULL;
    s32 damage = THROTTLE_RAM_BASE_DAMAGE + THROTTLE_RAM_DAMAGE_PER_LEVEL * st->throttleLevel;

    while ((other = hackLib_nextTarget(&query, other)) != NULL)
    {
        HackLibDamageResult result;

        // Each foe takes one ram until it is stunned.
        if (other->actionState == ACTOR_STATE_STUNNED)
        {
            continue;
        }
        result = skillExpansion_dealDamage(actor, other, damage, HACKLIB_DAMAGE_ABILITY,
                                           st->throttleRow);
        if (!result.hit)
        {
            continue;
        }
        hackLib_fxBurst(other, FX_SHAPE_DEBRIS, FX_COLOR_WHITE, FX_LAYER_FRONT);
        sfx_play(SFX_COMBAT_KNOCKDOWN);
        if (result.killed)
        {
            ratCatcher(playerIndex, actor);
        }
        else
        {
            hackLib_applyStun(other, THROTTLE_RAM_STUN_FRAMES);
        }
    }
}

/** Ride the throttled foe: the d-pad steers it into other foes while it chokes. */
static void tickThrottle(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    Actor *foe;
    Vector2Fp16 next;

    if (st->throttleFrames == 0)
    {
        return;
    }
    foe = throttledFoe(st);
    if (foe == NULL || --st->throttleFrames == 0)
    {
        endThrottle(st, actor);
        return;
    }
    next = foe->coords;
    if (st->steerKeys & DPAD_RIGHT)
    {
        next.x += THROTTLE_STEP << FP16_16_SHIFT;
    }
    if (st->steerKeys & DPAD_LEFT)
    {
        next.x -= THROTTLE_STEP << FP16_16_SHIFT;
    }
    if (st->steerKeys & DPAD_DOWN)
    {
        next.y += THROTTLE_STEP << FP16_16_SHIFT;
    }
    if (st->steerKeys & DPAD_UP)
    {
        next.y -= THROTTLE_STEP << FP16_16_SHIFT;
    }
    if ((next.x != foe->coords.x || next.y != foe->coords.y) && hackLib_canLandAt(foe, &next))
    {
        moveActor(foe, &next);
    }
    moveActor(actor, &foe->coords);
    if (st->throttleFrames % THROTTLE_RAM_FRAMES == 0)
    {
        ramFoes(playerIndex, st, actor, foe);
    }
    if (st->throttleFrames % THROTTLE_CHOKE_FRAMES == 0)
    {
        HackLibDamageResult result = skillExpansion_dealDamage(
            actor, foe,
            THROTTLE_CHOKE_BASE_DAMAGE + THROTTLE_CHOKE_DAMAGE_PER_LEVEL * st->throttleLevel,
            HACKLIB_DAMAGE_ABILITY, st->throttleRow);

        hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_GREEN, FX_LAYER_FRONT);
        if (result.killed)
        {
            ratCatcher(playerIndex, actor);
            endThrottle(st, actor);
        }
    }
}

static void removeBait(SmeagolState *st)
{
    if (hackLib_isActorRefValid(st->baitFx))
    {
        hackLib_fxRemove(st->baitFx.actor);
    }
    st->baitFx = (HackLibActorRef){0};
    st->baitFrames = 0;
}

static bool nearBait(SmeagolState *st, Actor *foe, s32 radius)
{
    return distanceSquared(&foe->coords, &st->baitPosition) <= radius * radius;
}

static void grabBait(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    s32 finders = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FINDERS_KEEPERS);
    SkillExpansionBurst burst = {
        .attacker = actor,
        .center = actor,
        .radius = BAIT_SURGE_RADIUS,
        .damage = BAIT_SURGE_BASE_DAMAGE + BAIT_SURGE_DAMAGE_PER_LEVEL * st->baitLevel,
        .stunFrames = BAIT_SURGE_STUN_FRAMES,
        .kind = HACKLIB_DAMAGE_ABILITY,
        .abilityRow = st->baitRow,
    };
    Actor *foe = NULL;

    removeBait(st);
    // Finders Keepers rolls before the surge so foes it kills still count.
    while (finders != 0 && (foe = findNextEnemy(actor, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && nearBait(st, foe, BAIT_SURGE_RADIUS) &&
            rng_rollRange(1, 100) <= FINDERS_KEEPERS_PERCENT_PER_LEVEL * finders)
        {
            loot_rollDrop(foe);
            hackLib_fxBurst(foe, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
    }
    addGrip(playerIndex, -BAIT_SURGE_GRIP);
    skillExpansion_spawnBurstFx(actor, FX_AOE_BURST_SUN, SFX_COMBAT_SUNBURST);
    castFx(actor, FX_SHAPE_RING_SPARKLES, FX_COLOR_GOLD, FX_LAYER_FRONT);
    skillExpansion_burstDamage(&burst);
    bark(playerIndex, "We has it! My precious!");
}

/** Foes crowd the dropped Ring until Smeagol grabs it back or Gollum loses patience. */
static void tickBait(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    s32 lure;
    Actor *foe = NULL;

    if (st->baitFrames == 0)
    {
        return;
    }
    if (st->baitArmFrames != 0)
    {
        if (!nearBait(st, actor, BAIT_LEAVE_RADIUS))
        {
            st->baitArmFrames--;
        }
    }
    else if (nearBait(st, actor, BAIT_PICKUP_RADIUS))
    {
        grabBait(playerIndex, st, actor);
        return;
    }
    if (--st->baitFrames == 0)
    {
        removeBait(st);
        bark(playerIndex, "Thief! Thief! Baggins!");
        startTakeover(playerIndex, st, actor);
        return;
    }
    if (st->baitFrames % BAIT_LURE_FRAMES != 0)
    {
        return;
    }
    lure = BAIT_LURE_BASE_RADIUS + BAIT_LURE_RADIUS_PER_LEVEL * st->baitLevel;
    while ((foe = findNextEnemy(actor, foe)) != NULL)
    {
        Vector2Fp16 next;

        if (!skillExpansion_isFoe(foe) || foe->actionState == ACTOR_STATE_STUNNED ||
            !nearBait(st, foe, lure) || nearBait(st, foe, BAIT_PICKUP_RADIUS))
        {
            continue;
        }
        next = foe->coords;
        next.x += foe->coords.x < st->baitPosition.x ? FP16_16_ONE : -FP16_16_ONE;
        next.y += foe->coords.y < st->baitPosition.y ? FP16_16_ONE : -FP16_16_ONE;
        if (hackLib_canLandAt(foe, &next))
        {
            moveActor(foe, &next);
        }
    }
}

/** Mark the nearest unopened chest or loot on the ground. */
static void tickSniff(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    s32 level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_SNIFF_IT_OUT);
    s32 radius = SNIFF_BASE_RADIUS + SNIFF_RADIUS_PER_LEVEL * level;
    s32 bestDistance = radius * radius;
    Actor *best = NULL;
    Actor *cursor;

    if (level == 0 || (st->sniffFrames != 0 && --st->sniffFrames != 0))
    {
        return;
    }
    st->sniffFrames = SNIFF_FRAMES;
    for (cursor = g_ActorListMain.head; cursor != NULL; cursor = cursor->next)
    {
        s32 distance;

        if ((cursor->flags.p & ACTOR_FLAG_PENDING_REMOVE) ||
            !(cursor->type == ACTOR_TYPE_LOOT_PILE ||
              (cursor->type == ACTOR_TYPE_CHEST && !cursor->as.chest.opened)))
        {
            continue;
        }
        distance = distanceSquared(&cursor->coords, &actor->coords);
        if (distance <= bestDistance)
        {
            best = cursor;
            bestDistance = distance;
        }
    }
    if (best != NULL)
    {
        hackLib_fxBurst(best, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
    }
}

// Smeagol's sprite palette drained to gaunt grey-green, with crushed shadows and pale highlights.
static const u16 GollumColors[16] = {
    0x7c1f, 0x5758, 0x4af5, 0x3e93, 0x3a51, 0x2dce, 0x298c, 0x216b,
    0x1d29, 0x14e7, 0x14e7, 0x0c84, 0x0863, 0x0423, 0x0442, 0x0021,
};

/** Load Smeagol's palette blended strength / GOLLUM_FADE_STEPS of the way to Gollum's. */
static void loadGollumColors(Actor *actor, u8 strength)
{
    const u16 *base = ObjPalettes[SMEAGOL_OBJ_PALETTE];
    u16 blended[16];
    u32 i;

    for (i = 0; i < 16; i++)
    {
        s32 mixed = 0;
        u32 shift;

        for (shift = 0; shift < 15; shift += 5)
        {
            s32 from = (base[i] >> shift) & 0x1f;
            s32 to = (GollumColors[i] >> shift) & 0x1f;

            mixed |= (from + (to - from) * strength / GOLLUM_FADE_STEPS) << shift;
        }
        blended[i] = mixed;
    }
    hackLib_fxTintSlot(actor->oam.paletteNumber, blended, 0, 0, 0, 0);
}

/** Smeagol keeps his stock colors except while Gollum holds the body. */
static void showState(Actor *actor, SmeagolState *st)
{
    u8 strength = st->tintStrength;

    if (st->takeoverFrames != 0 && strength < GOLLUM_FADE_STEPS)
    {
        strength++;
    }
    else if (st->takeoverFrames == 0 && strength != 0)
    {
        strength--;
    }
    if (strength != st->tintStrength ||
        (strength != 0 && (st->tintFrames == 0 || --st->tintFrames == 0)))
    {
        loadGollumColors(actor, strength);
        st->tintStrength = strength;
        st->tintFrames = TINT_REFRESH_FRAMES;
    }
    if (st->takeoverFrames != 0 && (st->sparkFrames == 0 || --st->sparkFrames == 0))
    {
        st->sparkFrames = TAKEOVER_SPARK_FRAMES;
        hackLib_fxBurst(actor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_SHADOW, FX_LAYER_BEHIND);
    }
}

void skillExpansion_smeagolReset(u8 playerIndex)
{
    if (!isSmeagol(playerIndex))
    {
        return;
    }
    memory_memset(state(playerIndex), 0, sizeof(SmeagolState));
}

void skillExpansion_smeagolCleanup(u8 playerIndex, Actor *actor)
{
    SmeagolState *st;

    if (!isSmeagol(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    if (st->hiding && actor != NULL)
    {
        actor->flags.p &= ~ACTOR_FLAG_HIDDEN;
    }
    removeBait(st);
    if (st->tintStrength != 0 && actor != NULL)
    {
        loadGollumColors(actor, 0);
        st->tintStrength = 0;
    }
    st->hiding = FALSE;
    st->takeoverFrames = st->playDeadFrames = st->throttleFrames = st->hideFrames = 0;
    st->throttled.actor = NULL;
}

void skillExpansion_smeagolTick(u8 playerIndex, Actor *actor)
{
    SmeagolState *st;

    if (!isSmeagol(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (st->barkFrames != 0)
    {
        st->barkFrames--;
    }
    if (st->freeFrames != 0)
    {
        st->freeFrames--;
    }
    if (st->hideFrames != 0)
    {
        st->hideFrames--;
    }
    if (st->takeoverFrames != 0)
    {
        tickTakeover(playerIndex, st, actor);
    }
    else
    {
        tickGrip(playerIndex, st, actor);
        announceGrip(playerIndex, st);
        if (st->grip >= GRIP_MAX && st->freeFrames == 0)
        {
            startTakeover(playerIndex, st, actor);
        }
    }
    tickPlayDead(playerIndex, st, actor);
    tickCatch(playerIndex, st, actor);
    tickThrottle(playerIndex, st, actor);
    tickBait(playerIndex, st, actor);
    tickSniff(playerIndex, st, actor);
    updateHiding(st, actor);
    showState(actor, st);
}

static u16 keysToward(Actor *from, Actor *to)
{
    s32 dx = (to->coords.x - from->coords.x) >> FP16_16_SHIFT;
    s32 dy = (to->coords.y - from->coords.y) >> FP16_16_SHIFT;
    u16 keys = 0;

    if (dx > TAKEOVER_DEADZONE)
    {
        keys |= DPAD_RIGHT;
    }
    else if (dx < -TAKEOVER_DEADZONE)
    {
        keys |= DPAD_LEFT;
    }
    if (dy > TAKEOVER_DEADZONE)
    {
        keys |= DPAD_DOWN;
    }
    else if (dy < -TAKEOVER_DEADZONE)
    {
        keys |= DPAD_UP;
    }
    return keys;
}

/** Gollum lunges at the nearest foe and claws it. A mashes to fight him off. */
static void takeoverInput(u8 playerIndex, SmeagolState *st, Actor *actor)
{
    u16 real = PLAYER_KEYS_CURRENT(playerIndex);
    u16 keep = real & (START_BUTTON | SELECT_BUTTON);
    u16 keys = 0;
    u16 pressed = 0;
    Actor *foe = nearestFoe(actor, TAKEOVER_SEEK_RADIUS);

    if (PLAYER_KEYS_PRESSED(playerIndex) & A_BUTTON)
    {
        st->mashPresses++;
    }
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WE_WANTS_IT) != 0 && (real & DPAD_ANY))
    {
        keys = real & DPAD_ANY;
    }
    else if (foe != NULL)
    {
        keys = keysToward(actor, foe);
    }
    if (foe != NULL &&
        distanceSquared(&foe->coords, &actor->coords) <= TAKEOVER_REACH * TAKEOVER_REACH &&
        st->takeoverFrames % TAKEOVER_ATTACK_FRAMES == 0)
    {
        keys |= B_BUTTON;
        pressed |= B_BUTTON;
    }
    PLAYER_KEYS_CURRENT(playerIndex) = keys | keep;
    PLAYER_KEYS_PRESSED(playerIndex) = pressed | (PLAYER_KEYS_PRESSED(playerIndex) & keep);
    PLAYER_KEYS_RELEASED(playerIndex) = 0;
}

void skillExpansion_smeagolInput(u8 playerIndex)
{
    SmeagolState *st;
    Actor *actor = PLAYER(playerIndex).ownerActor;
    u16 strip;

    if (!isSmeagol(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (st->takeoverFrames != 0)
    {
        takeoverInput(playerIndex, st, actor);
        return;
    }
    if (st->throttleFrames != 0)
    {
        // The d-pad steers the foe and Smeagol's hands are busy.
        st->steerKeys = PLAYER_KEYS_CURRENT(playerIndex) & DPAD_ANY;
        strip = DPAD_ANY | A_BUTTON | B_BUTTON | L_BUTTON | R_BUTTON;
    }
    else if (st->playDeadFrames != 0)
    {
        // B gets up with the x3 strike.
        if (PLAYER_KEYS_PRESSED(playerIndex) & B_BUTTON)
        {
            st->getUp = TRUE;
        }
        strip = DPAD_ANY;
    }
    else
    {
        return;
    }
    PLAYER_KEYS_CURRENT(playerIndex) &= ~strip;
    PLAYER_KEYS_PRESSED(playerIndex) &= ~strip;
    PLAYER_KEYS_RELEASED(playerIndex) &= ~strip;
}

s32 skillExpansion_smeagolDamage(u8 playerIndex, s32 damage, bool melee)
{
    SmeagolState *st;
    s32 level;

    if (!isSmeagol(playerIndex))
    {
        return damage;
    }
    st = state(playerIndex);
    level = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_HUNGRY);
    damage += damage * HUNGRY_PERCENT_PER_LEVEL * level * (st->grip / 10) / 100;
    if (st->takeoverFrames != 0)
    {
        damage *= TAKEOVER_DAMAGE_MULTIPLIER;
    }
    if (melee && st->strikeFrames != 0)
    {
        damage *= PLAY_DEAD_STRIKE_MULTIPLIER;
        st->striking = TRUE;
    }
    return damage;
}

void skillExpansion_smeagolOnHit(u8 playerIndex, Actor *victim, HackLibDamageResult result,
                                 bool melee)
{
    SmeagolState *st;

    if (!isSmeagol(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    if (st->striking && melee)
    {
        st->striking = FALSE;
        st->strikeFrames = 0;
        skillExpansion_spawnBurstFx(victim, FX_AOE_BURST_NIGHT, SFX_COMBAT_CRIT_HIT);
    }
    if (!result.killed)
    {
        return;
    }
    if (st->takeoverFrames != 0)
    {
        if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_WE_WANTS_IT) != 0)
        {
            s32 frames = st->takeoverFrames + WE_WANTS_IT_KILL_FRAMES;

            st->takeoverFrames = frames > WE_WANTS_IT_MAX_FRAMES ? WE_WANTS_IT_MAX_FRAMES : frames;
        }
    }
    else
    {
        addGrip(playerIndex, GRIP_KILL_GAIN);
    }
    if ((st->takeoverFrames != 0 || st->grip >= MY_PRECIOUS_GRIP) &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_MY_PRECIOUS) != 0)
    {
        loot_rollDrop(victim);
        hackLib_fxBurst(victim, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
    }
}

s32 skillExpansion_smeagolIncomingDamage(u8 playerIndex, s32 damage)
{
    SmeagolState *st;

    if (!isSmeagol(playerIndex) || damage <= 0)
    {
        return damage;
    }
    st = state(playerIndex);
    if (st->freeFrames != 0)
    {
        return 0;
    }
    if (st->playDeadFrames != 0)
    {
        st->getUp = TRUE;
    }
    if (st->takeoverFrames == 0)
    {
        addGrip(playerIndex, GRIP_HIT_GAIN);
    }
    return damage;
}

void skillExpansion_smeagolPlayDead(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SmeagolState *st = state(p);
    u8 level = castLevel(p);
    s32 nice = passiveLevel(p, SKILL_EXPANSION_PASSIVE_NICE_MASTER);

    st->playDeadFrames = PLAY_DEAD_BASE_FRAMES + PLAY_DEAD_FRAMES_PER_LEVEL * level;
    st->getUp = FALSE;
    addGrip(p, -PLAY_DEAD_GRIP);
    if (nice != 0)
    {
        s32 heal = PLAYER_STAT(p, STAT_MAX_HP) * NICE_MASTER_HEAL_PERCENT_PER_LEVEL * nice / 100;

        player_addHp(p, heal > 0 ? heal : 1, FALSE);
        hackLib_fxBurst(playerActor, FX_SHAPE_RING_SPARKLES, FX_COLOR_WHITE, FX_LAYER_FRONT);
    }
    castFx(playerActor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_SHADOW, FX_LAYER_FRONT);
    updateHiding(st, playerActor);
    skillExpansion_feedback(p, "Poor Smeagol... dead...");
}

void skillExpansion_smeagolCatchAFish(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SmeagolState *st = state(p);
    u8 level = castLevel(p);

    st->catchLevel = level;
    st->catchRow = PLAYER(p).castingActiveSkill;
    st->catchFrames = CATCH_BASE_FRAMES + CATCH_FRAMES_PER_LEVEL * level;
    st->catchesLeft = 1 + level / 2;
    hackLib_fxTimed(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_BLUE, st->catchFrames,
                    FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_OVAL_RING, FX_COLOR_BLUE, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_PLAYER_DODGE);
    skillExpansion_feedback(p, "Quick, quick! Catch it!");
}

SkillExpansionCastResult skillExpansion_smeagolThrottle(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SmeagolState *st = state(p);
    u8 level = castLevel(p);
    s32 frames = THROTTLE_BASE_FRAMES + THROTTLE_FRAMES_PER_LEVEL * level;
    Actor *foe = nearestFoe(playerActor, THROTTLE_REACH);

    if (foe == NULL || !hackLib_applyStun(foe, frames + 2))
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    st->throttled = hackLib_actorRef(foe);
    st->throttleFrames = frames;
    st->throttleLevel = level;
    st->throttleRow = PLAYER(p).castingActiveSkill;
    st->steerKeys = 0;
    addGrip(p, THROTTLE_GRIP);
    moveActor(playerActor, &foe->coords);
    castFx(playerActor, FX_SHAPE_TENDRIL_BURST, FX_COLOR_GREEN, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    skillExpansion_feedback(p, "Gollum! Gollum!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_smeagolSlink(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SmeagolState *st = state(p);
    u8 level = castLevel(p);
    Direction8 facing = playerActor->facing < 8 ? playerActor->facing : DIRECTION8_SOUTH;
    Vector2Fp16 origin = playerActor->coords;
    s32 distance;

    // The farthest floor ahead, whatever lies between.
    for (distance = SLINK_MAX_DISTANCE; distance >= SLINK_MIN_DISTANCE;
         distance -= SLINK_DISTANCE_STEP)
    {
        Vector2Fp16 landing = origin;

        landing.x += FacingVectors[facing].x * (distance << FP16_16_SHIFT);
        landing.y += FacingVectors[facing].y * (distance << FP16_16_SHIFT);
        if (hackLib_canLandAt(playerActor, &landing))
        {
            hackLib_fxAt(playerActor, &origin, FX_SHAPE_SMOKE_RISE, FX_COLOR_SHADOW);
            moveActor(playerActor, &landing);
            hackLib_fxBurst(playerActor, FX_SHAPE_SMOKE_SMALL, FX_COLOR_SHADOW, FX_LAYER_FRONT);
            st->hideFrames = SLINK_BASE_HIDE_FRAMES + SLINK_HIDE_FRAMES_PER_LEVEL * level;
            updateHiding(st, playerActor);
            addGrip(p, SLINK_GRIP);
            sfx_play(SFX_COMBAT_PLAYER_DODGE);
            return SKILL_CAST_SUCCESS;
        }
    }
    return SKILL_CAST_BLOCKED_LANDING;
}

void skillExpansion_smeagolDropThePrecious(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SmeagolState *st = state(p);

    st->baitLevel = castLevel(p);
    st->baitRow = PLAYER(p).castingActiveSkill;
    st->baitPosition = playerActor->coords;
    st->baitFrames = BAIT_FRAMES;
    st->baitArmFrames = BAIT_ARM_FRAMES;
    Actor *fx = hackLib_fxAt(playerActor, &st->baitPosition, FX_SHAPE_GROUND_POOL, FX_COLOR_GOLD);
    hackLib_fxHold(fx, BAIT_FRAMES);
    st->baitFx = hackLib_actorRef(fx);
    castFx(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, FX_LAYER_FRONT);
    updateHiding(st, playerActor);
    skillExpansion_feedback(p, "Take it... no! Precious!");
}

void skillExpansion_smeagolArgueWithYourself(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    SmeagolState *st = state(p);
    u8 level = castLevel(p);
    bool shout = st->grip >= ARGUE_SHOUT_GRIP;

    addGrip(p, -(ARGUE_BASE_GRIP + ARGUE_GRIP_PER_LEVEL * level));
    st->barkBand = 0;
    castFx(playerActor, FX_SHAPE_BUBBLE_RINGS, FX_COLOR_GREEN, FX_LAYER_FRONT);
    if (shout)
    {
        HackLibTargetQuery query = {
            .center = playerActor, .radius = ARGUE_SHOUT_RADIUS, .filter = isFoeTarget};
        Actor *foe = NULL;

        while ((foe = hackLib_nextTarget(&query, foe)) != NULL)
        {
            hackLib_applyStun(foe, ARGUE_SHOUT_BASE_STUN_FRAMES +
                                       ARGUE_SHOUT_STUN_FRAMES_PER_LEVEL * level);
        }
        sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
        bark(p, "Leave now and never come back!");
    }
    else
    {
        bark(p, "We don't need you!");
    }
}

bool32 skillExpansion_canCastSmeagolSkill(u8 playerIndex)
{
    SmeagolState *st = state(playerIndex);

    return st->takeoverFrames == 0 && st->throttleFrames == 0 && st->playDeadFrames == 0 &&
           st->baitFrames == 0 && st->catchFrames == 0;
}

asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global skillExpansion_original_input_update\n"
    ".thumb_func\n"
    "skillExpansion_original_input_update:\n"
    "    push {r4-r7, lr}\n"
    "    mov r7, r10\n"
    "    mov r6, r9\n"
    "    mov r5, r8\n"
    "    ldr r3, =input_update + 8\n"
    "    bx r3\n"
    "    .pool\n");

void skillExpansion_original_input_update(void);

/**
 * Read the keys, then let a Smeagol whose body is taken rewrite his.
 *
 * WRAPS: input_update.
 */
void skillExpansion_inputUpdate(void)
{
    u8 p;

    skillExpansion_original_input_update();
    if (g_SceneCurrent.id != SCENE_ID_GAMEPLAY || g_GameFlags.d.overlay)
    {
        return;
    }
    for (p = 0; p < 2; p++)
    {
        skillExpansion_smeagolInput(p);
        skillExpansion_gandalfInput(p);
        skillExpansion_legolasInput(p);
    }
}
