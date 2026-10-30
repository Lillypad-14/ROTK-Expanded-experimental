// Legolas's Two Towers actives.

#include "patches/legacy_skills/legacy_fx.h"

#if PC_EDITION && SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

#include "gba_io.h"
#include "input.h"
#include "rng.h"
#include "stats.h"
#include "variables.h"

// White Knives (Two Towers 0x0803B98C, 0x0801DA0C, the knives 0x08025C70): for 2 Spirit, Legolas
// slashes with his knives as his weapon. The Two Towers adds no damage of its own (the description's
// 4 + 1 per level is the knives'). It is ROTK's White Knives, which hits with the backpack knife as
// the weapon, without ROTK's stored damage.
static void whiteKnives(Actor *playerActor)
{
    skill_active_legolasWhiteKnives(playerActor);
}

static void castWhiteKnives(Actor *playerActor)
{
    skillExpansion_castStockCallback(playerActor);
    playerActor->as.combat.storedDamage = 0;
}

// Spread Fire (Two Towers 0x0803B934, 0x0801DEE0, an arrow 0x08026FA0, its tick 0x08027194 and hit
// 0x08010978): for 9 Spirit, 2 arrows + 1 per 4 levels (at most 7), fanned out by the angles below
// (32-way steps from Legolas's facing), each flying 21 ticks at speed 7.5 from a step ahead. An arrow
// hits the first foe it touches with Legolas's own attack, +1 accuracy per level, and is gone. Not
// ported: the glow on Legolas while he draws.
#define SPREAD_FIRE_ARROWS_BASE 2
#define SPREAD_FIRE_LEVELS_PER_ARROW 4
#define SPREAD_FIRE_ARROWS_MAX 7

static const s8 SpreadFireAngles[SPREAD_FIRE_ARROWS_MAX] = {-2, 2, 0, -4, 4, -1, 1};

static const u32 ArrowSprites[8] = {
    0x08329960u, 0x08329968u, 0x08329970u, 0x08329978u,
    0x08329980u, 0x08329978u, 0x08329970u, 0x08329968u,
};

static const LegacyThrownDef SpreadFireArrow = {
    .sprites = ArrowSprites,
    .speed = 0x78000,
    .lifetime = 21,
    .bonus = {0, 1},
    .bonusStat = STAT_ACCURACY,
    .flags = LEGACY_THROWN_STOP_ON_HIT | LEGACY_THROWN_START_AHEAD,
    .sound = LEGACY_SOUND_NONE,
};

static void castSpreadFire(Actor *playerActor)
{
    u32 arrows = SPREAD_FIRE_ARROWS_BASE + legacyFx_castingLevel(playerActor) / SPREAD_FIRE_LEVELS_PER_ARROW;

    if (arrows > SPREAD_FIRE_ARROWS_MAX)
    {
        arrows = SPREAD_FIRE_ARROWS_MAX;
    }
    for (u32 i = 0; i < arrows; i++)
    {
        legacyFx_castThrownToward(playerActor, &SpreadFireArrow, playerActor->facing * 4 + SpreadFireAngles[i]);
    }
}

// Suppress (Two Towers 0x0803C404, the arrow 0x0802730C, its hit 0x08011598; the knock 0x080274E4,
// its tick 0x08027624 and hit 0x08011754): for 3 Spirit, an arrow like Spread Fire's that hits the
// first foe it touches with Legolas's attack and is gone. Where it hits, a knock touches the foes
// within 35 pixels once, each knocked back along the arrow for 15 ticks on a roll of 0..100 at most
// 25 + 3 per level (the description says 25% + 5% per level). Not ported: the glow on Legolas.
#define SUPPRESS_RADIUS 35
#define SUPPRESS_KNOCK_TICKS 15
#define SUPPRESS_KNOCK 0x08052660u

static const LegacyScale SuppressChance = {25, 3};

static const u32 SuppressSprites[8] = {
    0x083299B0u, 0x083299B8u, 0x083299C0u, 0x083299C8u,
    0x083299D0u, 0x083299C8u, 0x083299C0u, 0x083299B8u,
};

static void knockTick(Actor *fx)
{
    Actor *foe = LEGACY_FX(fx)->target;

    // it knocks on the frame it appears only
    fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    if (foe != NULL)
    {
        fx->coords = foe->coords;
    }
    fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : SUPPRESS_KNOCK_TICKS;
    actor_unknown08045584(fx);
}

static void knockHit(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other) && rng_rollRange(0, 100) <= LEGACY_FX(fx)->amount)
    {
        actor_applyKnockback(other, fx, SUPPRESS_KNOCK_TICKS);
    }
}

static void suppressKnock(Actor *arrow, Actor *foe)
{
    Actor *fx = legacyFx_spawn(arrow->parentActor, SUPPRESS_KNOCK);

    if (fx == NULL)
    {
        return;
    }
    fx->coords = foe->coords;
    fx->facing = arrow->facing;
    fx->tickCallback = knockTick;
    fx->triggerCallbacks[0] = knockHit;
    fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY | ACTOR_FLAG_COLLISION_CHECK;
    fx->interactionRadius = SUPPRESS_RADIUS;
    fx->actionStateTimer = SUPPRESS_KNOCK_TICKS;
    LEGACY_FX(fx)->target = foe;
    LEGACY_FX(fx)->amount = legacyScale(SuppressChance, legacyFx_castingLevel(arrow->parentActor));
}

static const LegacyThrownDef SuppressArrow = {
    .sprites = SuppressSprites,
    .speed = 0x78000,
    .lifetime = 21,
    .flags = LEGACY_THROWN_STOP_ON_HIT | LEGACY_THROWN_START_AHEAD,
    .sound = LEGACY_SOUND_NONE,
    .onHit = suppressKnock,
};

static void castSuppress(Actor *playerActor)
{
    legacyFx_castThrown(playerActor, &SuppressArrow);
}

// Forest Friend (Two Towers 0x0803B8D4, the call 0x08026440, its tick 0x080265A8 and hit
// 0x0801154C; a hawk 0x08038A1C, its tick 0x08038B94): for 12 Spirit, a call on Legolas that sends a
// hawk at each of the first 2 + level / 2 foes within 75 pixels on its first tick. A hawk comes in
// from 160 pixels left or right of Legolas (at random) at a random height within 60, flies at its foe
// at speed 5, stuns it for 30 + 6 per level ticks when it reaches it (no damage; the description says
// injure) and flies on off the screen. It flies over walls.
#define FOREST_FRIEND_CALL 0x08052600u
#define FOREST_FRIEND_RADIUS 75
#define FOREST_FRIEND_HAWKS_BASE 2
#define FOREST_FRIEND_LEVELS_PER_HAWK 2
#define HAWK_SPEED 0x50000
#define HAWK_ENTRY_X (160 << 16)
#define HAWK_ENTRY_Y 60
#define HAWK_HOLD 30
#define HAWK_SCREEN_MARGIN 40
// actor_direction8TowardPoint: within the tolerance on both axes
#define DIRECTION8_ARRIVED 8
// a hawk's phase
#define HAWK_ENTERING 0
#define HAWK_SWOOPING 1
#define HAWK_STRIKING 2
#define HAWK_LEAVING 3

static const LegacyScale HawkStun = {30, 6};

static const u32 HawkFlying[8] = {
    0x0832E9FCu, 0x0832EA04u, 0x0832EA0Cu, 0x0832EA14u,
    0x0832EA1Cu, 0x0832EA14u, 0x0832EA0Cu, 0x0832EA04u,
};

static const u32 HawkStriking[8] = {
    0x0832E9D4u, 0x0832E9DCu, 0x0832E9E4u, 0x0832E9ECu,
    0x0832E9F4u, 0x0832E9ECu, 0x0832E9E4u, 0x0832E9DCu,
};

static void hawkHead(Actor *fx, Direction8 facing)
{
    fx->facing = facing;
    actor_setVelocityFromDirection8(&fx->velocity, fx->flags.p, facing, HAWK_SPEED);
    legacyFx_setSprite(fx, HawkFlying[facing]);
    legacyFx_faceSprite(fx);
}

// Placed on its first tick, as the eagle is: a hawk moved where it is spawned (in the call's
// trigger callback) is off somewhere else by its first tick.
static void hawkEnter(Actor *fx)
{
    Actor *foe = LEGACY_FX(fx)->target;

    fx->coords = fx->parentActor->coords;
    fx->coords.x += rng_rollUpTo(1) ? HAWK_ENTRY_X : -HAWK_ENTRY_X;
    fx->coords.y += rng_rollRange(-HAWK_ENTRY_Y, HAWK_ENTRY_Y) << 16;
    fx->flags.p |= ACTOR_FLAG_RENDER | ACTOR_FLAG_ANIMATION_PLAYING;
    LEGACY_FX(fx)->phase = HAWK_SWOOPING;
    hawkHead(fx, actor_direction8TowardPoint(fx->coords.x, fx->coords.y, foe->coords.x, foe->coords.y,
                                             HAWK_SPEED));
}

static void hawkTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    Actor *foe = state->target;

    if (state->phase == HAWK_ENTERING)
    {
        hawkEnter(fx);
    }
    else if (state->phase == HAWK_SWOOPING)
    {
        if (foe->flags.p & ACTOR_FLAG_PENDING_REMOVE)
        {
            state->phase = HAWK_LEAVING;
        }
        else
        {
            Direction8 facing = actor_direction8TowardPoint(fx->coords.x, fx->coords.y, foe->coords.x,
                                                            foe->coords.y, HAWK_SPEED);

            if (facing == DIRECTION8_ARRIVED)
            {
                fx->velocity.x = 0;
                fx->velocity.y = 0;
                state->phase = HAWK_STRIKING;
                legacyFx_setSprite(fx, HawkStriking[fx->facing]);
                legacyFx_faceSprite(fx);
                foe->actionState = ACTOR_STATE_STUNNED;
                foe->actionStateTimer = state->amount;
                foe->field_0x64[0] |= 2;
            }
            else if (facing != fx->facing)
            {
                hawkHead(fx, facing);
            }
        }
    }
    else if (state->phase == HAWK_STRIKING)
    {
        if (legacyFx_animationEnded(fx))
        {
            state->phase = HAWK_LEAVING;
            hawkHead(fx, fx->facing);
        }
    }
    else if (!legacyFx_onScreen(fx, HAWK_SCREEN_MARGIN))
    {
        legacyFx_end(fx);
    }
    if (fx->actionState != ACTOR_STATE_UNKNOWN_04)
    {
        fx->actionStateTimer = HAWK_HOLD;
    }
    legacyFx_finishTick(fx);
}

static void sendHawk(Actor *call, Actor *foe)
{
    Actor *caster = call->parentActor;
    Actor *fx = legacyFx_spawn(caster, 0);

    if (fx == NULL)
    {
        return;
    }
    fx->coords = caster->coords;
    fx->flags.p &= ~(ACTOR_FLAG_TILE_COLLISION | ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_RENDER |
                     ACTOR_FLAG_ANIMATION_PLAYING);
    fx->tickCallback = hawkTick;
    fx->actionStateTimer = HAWK_HOLD;
    LEGACY_FX(fx)->target = foe;
    LEGACY_FX(fx)->amount = legacyScale(HawkStun, legacyFx_castingLevel(caster));
    LEGACY_FX(fx)->phase = HAWK_ENTERING;
}

static void callTick(Actor *fx)
{
    // it picks its foes on its first tick only
    if (LEGACY_FX(fx)->phase != 0)
    {
        fx->flags.p &= ~ACTOR_FLAG_COLLISION_CHECK;
    }
    LEGACY_FX(fx)->phase = 1;
    fx->coords = fx->parentActor->coords;
    fx->actionStateTimer = legacyFx_animationEnded(fx) ? 0 : HAWK_HOLD;
    legacyFx_finishTick(fx);
}

static void callPick(Actor *fx, Actor *other)
{
    if (legacyFx_isFoe(other) && LEGACY_FX(fx)->count != 0)
    {
        LEGACY_FX(fx)->count--;
        sendHawk(fx, other);
    }
}

static void castForestFriend(Actor *playerActor)
{
    Actor *fx = legacyFx_spawn(playerActor, FOREST_FRIEND_CALL);

    if (fx == NULL)
    {
        return;
    }
    fx->tickCallback = callTick;
    fx->triggerCallbacks[0] = callPick;
    fx->flags.p |= ACTOR_FLAG_RADIUS_PROXIMITY | ACTOR_FLAG_COLLISION_CHECK;
    fx->interactionRadius = FOREST_FRIEND_RADIUS;
    fx->actionStateTimer = HAWK_HOLD;
    LEGACY_FX(fx)->count =
        FOREST_FRIEND_HAWKS_BASE + legacyFx_castingLevel(playerActor) / FOREST_FRIEND_LEVELS_PER_HAWK;
}

// Overdraw (Two Towers 0x0803C3B4, the draw 0x0801DCE8, its glow 0x0802686C and tick 0x08026984, the
// arrow 0x08026678 and hit 0x08010904): for 6 Spirit, Legolas draws, then holds the draw for as long
// as the button that cast it stays down, a glow under him growing every 30 ticks (4 stages). On release
// one arrow flies at the first foe on screen in front of him (else straight ahead), speed 7.5 for 21
// ticks, hitting each foe it passes with Legolas's attack + level x (ticks held / 30 + 1) + 2 damage
// and stopping at a foe that does not flinch. The hold keeps Spread Fire's pose frozen at full draw.
#define OVERDRAW_STAGE_TICKS 30
#define OVERDRAW_STAGES 4
#define OVERDRAW_BONUS_BASE 2
#define OVERDRAW_HOLD_BUTTONS (A_BUTTON | B_BUTTON | R_BUTTON)
// the cast's recovery, topped up while the draw is held, keeps Legolas in his drawn pose
#define OVERDRAW_HOLD_RECOVERY 2
// the glow lies under Legolas: a pixel above him sorts it behind
#define OVERDRAW_GLOW_BEHIND (1 << 16)

static const u32 OverdrawGlow[OVERDRAW_STAGES] = {0x08052610u, 0x08052620u, 0x08052630u, 0x08052640u};

static const u32 OverdrawSprites[8] = {
    0x08329888u, 0x08329890u, 0x08329898u, 0x083298A0u,
    0x083298A8u, 0x083298A0u, 0x08329898u, 0x08329890u,
};

static const LegacyThrownDef OverdrawArrow = {
    .sprites = OverdrawSprites,
    .speed = 0x78000,
    .lifetime = 21,
    .flags = LEGACY_THROWN_START_AHEAD,
    .sound = LEGACY_SOUND_NONE,
};

// a foe Two Towers counts as in front: within one 8-way step of the facing (0x0801F2B0)
static bool32 inFront(Actor *archer, Actor *foe)
{
    Direction8 toward = actor_direction8TowardPoint(archer->coords.x, archer->coords.y, foe->coords.x,
                                                    foe->coords.y, 0);
    u8 turn = (toward - archer->facing) & 7;

    return toward < 8 && (turn <= 1 || turn == 7);
}

static u8 overdrawAim(Actor *archer)
{
    Actor *other;

    for (other = g_ActorListMain.head; other != NULL; other = other->next)
    {
        if ((other->flags.p & ACTOR_FLAG_ON_SCREEN) && legacyFx_isFoe(other) && inFront(archer, other) &&
            other->actionState != ACTOR_STATE_UNKNOWN_11 && other->actionState != ACTOR_STATE_UNKNOWN_15)
        {
            Vector2Fp16 toward = {other->coords.x - archer->coords.x, other->coords.y - archer->coords.y};

            return math_vector2_direction32(toward);
        }
    }
    return archer->facing * 4;
}

static void loose(Actor *fx, Actor *archer)
{
    Actor *arrow = legacyFx_castThrownToward(archer, &OverdrawArrow, overdrawAim(archer));

    if (arrow != NULL)
    {
        LEGACY_FX(arrow)->amount =
            legacyFx_castingLevel(archer) * (LEGACY_FX(fx)->amount / OVERDRAW_STAGE_TICKS + 1) + OVERDRAW_BONUS_BASE;
    }
    archer->flags.p |= ACTOR_FLAG_ANIMATION_PLAYING;
    legacyFx_end(fx);
}

static void drawTick(Actor *fx)
{
    LegacyFxState *state = LEGACY_FX(fx);
    Actor *archer = fx->parentActor;
    u8 stage;

    if (archer->actionState != ACTOR_STATE_ACTIVE_SKILL_CAST_ANIMATION &&
        archer->actionState != ACTOR_STATE_ACTIVE_SKILL_CAST_RECOVERY)
    {
        // knocked out of the draw: no arrow
        archer->flags.p |= ACTOR_FLAG_ANIMATION_PLAYING;
        legacyFx_end(fx);
    }
    else if (!(PLAYER_KEYS_CURRENT(archer->as.combat.playerIndex) & state->count))
    {
        loose(fx, archer);
    }
    else
    {
        archer->flags.p &= ~ACTOR_FLAG_ANIMATION_PLAYING;
        if (archer->actionState == ACTOR_STATE_ACTIVE_SKILL_CAST_RECOVERY &&
            archer->actionStateTimer < OVERDRAW_HOLD_RECOVERY)
        {
            archer->actionStateTimer = OVERDRAW_HOLD_RECOVERY;
        }
        if (state->amount != 0xFFFF)
        {
            state->amount++;
        }
        stage = state->amount / OVERDRAW_STAGE_TICKS;
        if (stage >= OVERDRAW_STAGES)
        {
            stage = OVERDRAW_STAGES - 1;
        }
        if (stage != state->phase || legacyFx_animationEnded(fx))
        {
            state->phase = stage;
            legacyFx_setSprite(fx, OverdrawGlow[stage]);
        }
        fx->coords.x = archer->coords.x;
        fx->coords.y = archer->coords.y - OVERDRAW_GLOW_BEHIND;
        fx->actionStateTimer = OVERDRAW_STAGE_TICKS;
    }
    legacyFx_finishTick(fx);
}

static void castOverdraw(Actor *playerActor)
{
    Actor *fx = legacyFx_spawn(playerActor, OverdrawGlow[0]);
    u16 held = PLAYER_KEYS_CURRENT(playerActor->as.combat.playerIndex) & OVERDRAW_HOLD_BUTTONS;

    if (fx == NULL)
    {
        return;
    }
    legacyFx_place(fx, playerActor->coords);
    fx->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
    fx->tickCallback = drawTick;
    fx->actionStateTimer = OVERDRAW_STAGE_TICKS;
    // the button that cast it, still down: a tap looses at once
    LEGACY_FX(fx)->count = held;
    playerActor->flags.p &= ~ACTOR_FLAG_ANIMATION_PLAYING;
}

// Summon Gimli (Two Towers 0x0803B9E4, 0x0801DB2C): for 25 Spirit, Gimli fights beside Legolas for 300 +
// 60 per level ticks, with his axe.
static const u32 GimliWalk[5] = {0x08328F6Cu, 0x08328F74u, 0x08328F7Cu, 0x08328F84u, 0x08328F8Cu};
static const u32 GimliIdle[5] = {0x08328FBCu, 0x08328FC4u, 0x08328FCCu, 0x08328FD4u, 0x08328FDCu};
static const u32 GimliAttack[5] = {0x08328FE4u, 0x08328FECu, 0x08328FF4u, 0x08328FFCu, 0x08329004u};
static const u32 AxeWalk[5] = {0x08330938u, 0x08330940u, 0x08330948u, 0x08330950u, 0x08330958u};
static const u32 AxeIdle[5] = {0x08330988u, 0x08330990u, 0x08330998u, 0x083309A0u, 0x083309A8u};
static const u32 AxeAttack[5] = {0x083309B0u, 0x083309B8u, 0x083309C0u, 0x083309C8u, 0x083309D0u};

static const LegacyAllyDef Gimli = {
    .walk = GimliWalk,
    .idle = GimliIdle,
    .attack = GimliAttack,
    .weaponWalk = AxeWalk,
    .weaponIdle = AxeIdle,
    .weaponAttack = AxeAttack,
    .hit = LEGACY_ALLY_HIT_WEAPON,
    .range = 35,
    .paletteId = 25,
    .lifetime = {300, 60},
};

static void castSummonGimli(Actor *playerActor)
{
    legacyFx_castAlly(playerActor, &Gimli);
}

// ROTK's rows the actives take their cast poses from
// ROTK's White Knives needs a knife in the backpack; Legolas always has his in The Two Towers
static const SkillExpansionCastPolicy WhiteKnivesPolicy = {
    ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES + 1, ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES, SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionActiveDef WhiteKnivesRow = {
    .stockCallback = whiteKnives,
    .baseRow = ACTIVE_SKILL_LEGOLAS_WHITE_KNIVES,
    .policy = &WhiteKnivesPolicy,
};
static const SkillExpansionActiveDef SpreadFireRow = {.baseRow = ACTIVE_SKILL_LEGOLAS_SPREAD_FIRE};
// Friend of Mirkwood's pose without its one-summon rule
static const SkillExpansionCastPolicy CallPolicy = {
    ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD + 1, ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    SKILL_EXPANSION_RULE_NONE, 0};
static const SkillExpansionActiveDef CallRow = {
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &CallPolicy,
};
static const SkillExpansionActiveDef SummonRow = {
    .baseRow = ACTIVE_SKILL_LEGOLAS_FRIEND_OF_MIRKWOOD,
    .policy = &CallPolicy,
    .canCast = legacyFx_allyCanCast,
};

const LegacyActive LegacyLegolasActives[SKILL_EXPANSION_LEGACY_ACTIVE_COUNT] = {
    [0] = {&SpreadFireRow, 6, castOverdraw},
    [1] = {&SpreadFireRow, 3, castSuppress},
    [2] = {&SpreadFireRow, 9, castSpreadFire},
    [3] = {&WhiteKnivesRow, 2, castWhiteKnives},
    [4] = {&CallRow, 12, castForestFriend},
    [5] = {&SummonRow, 25, castSummonGimli},
};

#endif
