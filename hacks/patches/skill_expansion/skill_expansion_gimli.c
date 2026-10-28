#include "patches/skill_expansion/skill_expansion_gimli.h"
#include "enemy.h"
#include "fx.h"
#include "game.h"
#include "lib/fx_kit.h"
#include "lib/hud_meter.h"
#include "lib/movement.h"
#include "lib/status.h"
#include "lib/summon.h"
#include "lib/targeting.h"
#include "memory.h"
#include "patches/skill_expansion/skill_expansion_abilities.h"
#include "patches/skill_expansion/skill_expansion_elite.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "sfx.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

#define FRAMES_PER_SECOND 30

// Cast visuals step every CAST_FX_TICKS so they read instead of flashing by.
#define CAST_FX_TICKS 3

// Stock kill counter cap and its attribute point reward.
#define KILL_COUNT_MAX 0xffff
#define KILL_REWARD_STEP 500
#define KILL_REWARD_POINTS 2

// New kills handled per tick. More wait for the next tick.
#define MAX_KILLS_PER_TICK 8

#define NO_ROW 0xff
#define NO_GRUDGE 0xff

// Baruk Khazad! (Stone)
#define BARUK_RADIUS 80
#define BARUK_BASE_STUN_FRAMES FRAMES_PER_SECOND
#define BARUK_STUN_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 5)
#define BARUK_BASE_FRAMES (4 * FRAMES_PER_SECOND)
#define BARUK_FRAMES_PER_LEVEL FRAMES_PER_SECOND
#define BARUK_PERCENT_PER_FOE 4
#define BARUK_MAX_PERCENT 40

// Hold the Deep (Stone)
#define HOLD_FRAMES (3 * FRAMES_PER_SECOND)
#define HOLD_TAKEN_PERCENT 50
#define HOLD_RADIUS 56
#define HOLD_BASE_DAMAGE 10
#define HOLD_DAMAGE_PER_LEVEL 4
#define HOLD_BASE_RELEASE_PERCENT 60
#define HOLD_RELEASE_PERCENT_PER_LEVEL 10
#define HOLD_MAX_STORED 400
#define HOLD_STUN_FRAMES FRAMES_PER_SECOND

// Cleave the Weak (Fury)
#define CLEAVE_REACH 40
#define CLEAVE_BASE_PERCENT 15
#define CLEAVE_PERCENT_PER_LEVEL 3
#define CLEAVE_BASE_DAMAGE 8
#define CLEAVE_DAMAGE_PER_LEVEL 4
#define CLEAVE_EXECUTE_DAMAGE 0x7fff

// Nobody Tosses a Dwarf (Tally)
#define TOSS_REACH 112
#define TOSS_LANDING_DISTANCE 14
#define TOSS_RADIUS 40
#define TOSS_BASE_DAMAGE 10
#define TOSS_DAMAGE_PER_LEVEL 5
#define TOSS_STUN_FRAMES FRAMES_PER_SECOND

// Book of Grudges (Tally)
#define GRUDGE_REACH 128
#define GRUDGE_PERCENT_PER_LEVEL 10

// Still Counting (Fury)
#define COUNTING_BASE_FRAMES (12 * FRAMES_PER_SECOND)
#define COUNTING_FRAMES_PER_LEVEL (2 * FRAMES_PER_SECOND)
#define COUNTING_FRAMES_PER_KILL FRAMES_PER_SECOND
#define COUNTING_HEAL_PERCENT 3
#define COUNTING_RADIUS 72
#define COUNTING_DAMAGE_PER_KILL 4
#define COUNTING_MAX_DAMAGE 300
#define COUNTING_STUN_FRAMES FRAMES_PER_SECOND

// Passives
#define MAIL_RADIUS 48
#define MAIL_PERCENT_PER_LEVEL 2
#define MAIL_MAX_FOES 4
#define NOT_WITHOUT_ME_RADIUS 96
#define NOT_WITHOUT_ME_PERCENT_PER_LEVEL 8
#define GRUDGEBEARER_PERCENT_PER_LEVEL 4
#define BLOOD_FRAMES_PER_LEVEL (FRAMES_PER_SECOND / 2)
#define BLOOD_MAX_FRAMES (30 * FRAMES_PER_SECOND)
#define WAGER_PERCENT_PER_LEVEL 5
#define WAGER_STREAK_KILLS 3
#define WAGER_STREAK_FRAMES (5 * FRAMES_PER_SECOND)
#define WAGER_FRAMES (8 * FRAMES_PER_SECOND)
#define ORC_COUNT_KILLS 10
#define ORC_COUNT_SPIRIT_PER_LEVEL 4
#define FORTY_TWO_KILLS 42
#define FORTY_TWO_COUNTDOWN 2
#define FORTY_TWO_RADIUS 64
#define FORTY_TWO_BASE_DAMAGE 15
#define FORTY_TWO_DAMAGE_PER_LEVEL 5
#define FORTY_TWO_STUN_FRAMES FRAMES_PER_SECOND

// Charge bar: kills fill it, a full bar empowers the next active skill.
#define CHARGE_KILLS 10
#define EMPOWER_FRAMES (5 * FRAMES_PER_SECOND)
#define EMPOWER_PERCENT 50
#define BAR_PULSE_FRAMES 8

// Dwarven Rage's buff and aura fx descriptor rows.
#define RAGE_FX_TYPE 3
#define RAGE_AURA_FX_TYPE 8

typedef struct GimliState
{
    // Hold the Deep root position
    Vector2Fp16 holdPos;

    // Tally already handled by the tick
    u16 lastKills;

    // Hold the Deep frames left
    u16 holdFrames;

    // damage taken during Hold the Deep
    u16 holdStored;

    // Baruk Khazad! guard frames left
    u16 barukFrames;

    // Still Counting frames left
    u16 countingFrames;

    // Friendly Wager buff frames left
    u16 wagerFrames;

    // frames left to finish the Friendly Wager streak
    u16 streakFrames;

    // own kills in the current Friendly Wager streak
    u8 streakKills;

    // Empowered damage frames left
    u16 empowerFrames;

    // kills toward the charge, 0..CHARGE_KILLS
    u8 chargeKills;

    // HUD bar glint phase while charged
    u8 barPulseFrames;

    // TRUE while the cast in progress spends the charge
    bool empowering;

    // kills during Still Counting
    u8 countingKills;

    // foes cowed by Baruk Khazad!
    u8 barukCowed;

    // Hold the Deep level at cast
    u8 holdLevel;

    // row Hold the Deep was cast from
    u8 holdRow;

    // row Still Counting was cast from
    u8 countingRow;

    // grudged EnemyType, or NO_GRUDGE
    u8 grudgeType;

    // Book of Grudges level at cast
    u8 grudgeLevel;

    // row whose cooldown Cleave the Weak resets next tick, or NO_ROW
    u8 cleaveRow;

    // TRUE while Dwarven Rage is up
    bool raging;

    // TRUE once Durin's Folk Don't Fall saved Gimli in this Rage
    bool durinUsed;

    // Still Counting kill count text
    char countText[6];
} GimliState;

_Static_assert(sizeof(GimliState) <= sizeof(skillExpansion_heroState[0]),
               "GimliState exceeds the hero state");

static inline GimliState *state(u8 playerIndex)
{
    return (GimliState *)skillExpansion_heroStateClaim(playerIndex, HERO_ID_GIMLI,
                                                    PLAYER(playerIndex).heroId);
}

static inline bool isGimli(u8 playerIndex)
{
    return playerIndex < 2 && PLAYER(playerIndex).heroId == HERO_ID_GIMLI &&
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

static inline s32 percentOf(s32 value, s32 percent)
{
    return value * percent / 100;
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

static s32 distanceSquared(const Actor *a, const Actor *b)
{
    s32 dx = (a->coords.x - b->coords.x) >> FP16_16_SHIFT;
    s32 dy = (a->coords.y - b->coords.y) >> FP16_16_SHIFT;

    return dx * dx + dy * dy;
}

static u32 foesNear(Actor *center, s32 radius)
{
    Actor *foe = NULL;
    u32 count = 0;

    while ((foe = findNextEnemy(center, foe)) != NULL)
    {
        if (skillExpansion_isFoe(foe) && distanceSquared(center, foe) <= radius * radius)
        {
            count++;
        }
    }
    return count;
}

static u32 burst(Actor *actor, s32 radius, s32 damage, s32 stunFrames, u8 row)
{
    SkillExpansionBurst hit = {
        .attacker = actor,
        .center = actor,
        .radius = radius,
        .damage = damage > 0xffff ? 0xffff : damage,
        .stunFrames = stunFrames,
        .kind = HACKLIB_DAMAGE_ABILITY,
        .abilityRow = row,
    };

    return skillExpansion_burstDamage(&hit);
}

bool32 skillExpansion_gimliUsesTally(u8 playerIndex)
{
    return isGimli(playerIndex);
}

void skillExpansion_gimliAddTally(u8 playerIndex, u32 kills)
{
    Player *player = &PLAYER(playerIndex);
    u32 before = player->killCount;
    u32 after = before + kills;

    if (after > KILL_COUNT_MAX)
    {
        after = KILL_COUNT_MAX;
    }
    player->killCount = after;
    if (isGimli(playerIndex))
    {
        state(playerIndex)->lastKills += after - before;
    }
    if (after / KILL_REWARD_STEP != before / KILL_REWARD_STEP)
    {
        player->attributePoints +=
            KILL_REWARD_POINTS * (after / KILL_REWARD_STEP - before / KILL_REWARD_STEP);
        if (playerIndex == ACTIVE_PLAYER_INDEX)
        {
            player->statusFlags.p |= PLAYER_STATUS_FLAG_UNSPENT_POINTS;
        }
        if (g_PlayerHuds[playerIndex].skillPanelState == PLAYERHUD_SKILL_PANEL_IDLE)
        {
            g_PlayerHuds[playerIndex].skillPanelState = PLAYERHUD_SKILL_PANEL_DRAW_UNSPENT_POINTS;
        }
    }
}

void skillExpansion_gimliRaiseTally(u8 playerIndex, u16 kills)
{
    if (playerIndex >= 2 || PLAYER(playerIndex).killCount >= kills)
    {
        return;
    }
    PLAYER(playerIndex).killCount = kills;
    if (isGimli(playerIndex))
    {
        state(playerIndex)->lastKills = kills;
    }
}

static void clearState(u8 playerIndex)
{
    GimliState *st = state(playerIndex);

    memory_memset(st, 0, sizeof(*st));
    st->lastKills = PLAYER(playerIndex).killCount;
    st->grudgeType = NO_GRUDGE;
    st->cleaveRow = NO_ROW;
    st->holdRow = NO_ROW;
    st->countingRow = NO_ROW;
}

void skillExpansion_gimliReset(u8 playerIndex)
{
    if (!isGimli(playerIndex))
    {
        return;
    }
    clearState(playerIndex);
}

void skillExpansion_gimliCleanup(u8 playerIndex, Actor *actor)
{
    (void)actor;
    if (!isGimli(playerIndex))
    {
        return;
    }
    clearState(playerIndex);
}

static bool isRageBuff(const Actor *fx, const Actor *playerActor)
{
    return fx->parentActor == playerActor &&
           fx->tickCallback == (ActorCallback)skill_active_tickGimliDwarvenRage &&
           fx->as.gatePanel.tileSet == RAGE_FX_TYPE;
}

/** The Dwarven Rage buff and aura on Gimli gain frames, up to BLOOD_MAX_FRAMES left. */
static void extendRage(Actor *playerActor, s32 frames)
{
    Actor *fx;

    for (fx = g_ActorListMain.head; fx != NULL; fx = fx->next)
    {
        bool aura = fx->parentActor == playerActor &&
                    fx->tickCallback == (ActorCallback)(void *)fx_tickTimedAttachment &&
                    fx->as.gatePanel.tileSet == RAGE_AURA_FX_TYPE;
        s32 left;

        if (!aura && !isRageBuff(fx, playerActor))
        {
            continue;
        }
        left = fx->as.combat.venomDuration + frames;
        fx->as.combat.venomDuration = left > BLOOD_MAX_FRAMES ? BLOOD_MAX_FRAMES : left;
    }
}

static void fortyTwo(u8 playerIndex, Actor *actor)
{
    s32 level = *skillExpansion_activeLevel(playerIndex, ACTIVE_SKILL_GIMLI_EARTH_SHATTER);

    castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
    castFx(actor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_GOLD, FX_LAYER_FRONT);
    sfx_play(SFX_COMBAT_TRIPLE_DAMAGE);
    burst(actor, FORTY_TWO_RADIUS, FORTY_TWO_BASE_DAMAGE + FORTY_TWO_DAMAGE_PER_LEVEL * level,
          FORTY_TWO_STUN_FRAMES, ACTIVE_SKILL_GIMLI_EARTH_SHATTER);
    player_addSpirit(playerIndex, PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT));
    skillExpansion_feedback(playerIndex, "Forty-two!");
}

static void formatCount(char *text, u32 count)
{
    char digits[4];
    u32 n = 0;
    u32 i = 0;

    do
    {
        digits[n++] = '0' + count % 10;
        count /= 10;
    } while (count != 0 && n < sizeof(digits));
    while (n != 0)
    {
        text[i++] = digits[--n];
    }
    text[i++] = '!';
    text[i] = '\0';
}

/** One kill added to the Tally by anyone in the party. */
static void onTallyKill(u8 playerIndex, GimliState *st, Actor *actor, u32 tally)
{
    s32 blood = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_BLOOD_ON_THE_AXE);
    s32 orcCount = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_ORC_COUNT);
    bool charged = st->chargeKills < CHARGE_KILLS && ++st->chargeKills == CHARGE_KILLS;

    if (st->countingFrames != 0)
    {
        s32 heal = percentOf(PLAYER_STAT(playerIndex, STAT_MAX_HP), COUNTING_HEAL_PERCENT);
        u32 frames = st->countingFrames + COUNTING_FRAMES_PER_KILL;

        st->countingFrames = frames > 0xffff ? 0xffff : frames;
        if (st->countingKills < 0xff)
        {
            st->countingKills++;
        }
        player_addHp(playerIndex, heal > 0 ? heal : 1, FALSE);
        formatCount(st->countText, st->countingKills);
        skillExpansion_feedback(playerIndex, st->countText);
    }
    if (blood != 0 && st->raging)
    {
        extendRage(actor, BLOOD_FRAMES_PER_LEVEL * blood);
        hackLib_fxBurst(actor, FX_SHAPE_SPARK, FX_COLOR_BLOOD, FX_LAYER_FRONT);
    }
    if (tally % GIMLI_TALLY_KILLS_PER_MARK == 0)
    {
        if (!charged)
        {
            skillExpansion_feedback(playerIndex, "Tally mark!");
        }
        if (orcCount != 0)
        {
            player_addSpirit(playerIndex, ORC_COUNT_SPIRIT_PER_LEVEL * orcCount);
            hackLib_fxBurst(actor, FX_SHAPE_SPARK, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
    }
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FORTY_TWO) != 0)
    {
        static const char *const Countdown[FORTY_TWO_COUNTDOWN] = {"Forty-two in 1...",
                                                                   "Forty-two in 2..."};
        u32 left = FORTY_TWO_KILLS - tally % FORTY_TWO_KILLS;

        if (left == FORTY_TWO_KILLS)
        {
            fortyTwo(playerIndex, actor);
        }
        else if (left <= FORTY_TWO_COUNTDOWN)
        {
            skillExpansion_feedback(playerIndex, Countdown[left - 1]);
        }
    }
    if (charged)
    {
        sfx_play(SFX_SKILL_TREE_BUY_ACTIVE);
        skillExpansion_feedback(playerIndex, "Next skill empowered!");
    }
}

static void endHold(u8 playerIndex, GimliState *st, Actor *actor)
{
    s32 damage = HOLD_BASE_DAMAGE + HOLD_DAMAGE_PER_LEVEL * st->holdLevel +
                 percentOf(st->holdStored, HOLD_BASE_RELEASE_PERCENT +
                                               HOLD_RELEASE_PERCENT_PER_LEVEL * st->holdLevel);

    castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    castFx(actor, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_EMBER, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    burst(actor, HOLD_RADIUS, damage, HOLD_STUN_FRAMES, st->holdRow);
    st->holdStored = 0;
    skillExpansion_feedback(playerIndex, "The Deep answers!");
}

static void endCounting(u8 playerIndex, GimliState *st, Actor *actor)
{
    s32 damage = COUNTING_DAMAGE_PER_KILL * st->countingKills;

    if (damage > COUNTING_MAX_DAMAGE)
    {
        damage = COUNTING_MAX_DAMAGE;
    }
    if (damage != 0)
    {
        castFx(actor, FX_SHAPE_SHOCKWAVE, FX_COLOR_NIGHT, FX_LAYER_BEHIND);
        castFx(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_BLOOD, FX_LAYER_FRONT);
        sfx_play(SFX_COMBAT_SUNBURST);
        burst(actor, COUNTING_RADIUS, damage, COUNTING_STUN_FRAMES, st->countingRow);
    }
    st->countingKills = 0;
    skillExpansion_feedback(playerIndex, "That still only counts as one!");
}

void skillExpansion_gimliTick(u8 playerIndex, Actor *actor)
{
    GimliState *st;
    u32 tally = PLAYER(playerIndex).killCount;
    u32 handled = 0;
    bool raging;

    if (!isGimli(playerIndex) || actor == NULL)
    {
        return;
    }
    st = state(playerIndex);
    raging = PLAYER(playerIndex).combatFlags.d.dwarvenRage;
    if (raging && !st->raging &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DURINS_FOLK) != 0)
    {
        hackLib_fxBurst(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
        skillExpansion_feedback(playerIndex, "Durin's ward is up!");
    }
    if (!raging)
    {
        st->durinUsed = FALSE;
    }
    st->raging = raging;
    if (st->wagerFrames != 0)
    {
        st->wagerFrames--;
    }
    if (st->streakFrames != 0 && --st->streakFrames == 0)
    {
        st->streakKills = 0;
    }
    if (tally < st->lastKills)
    {
        st->lastKills = tally;
    }
    while (st->lastKills < tally && handled++ < MAX_KILLS_PER_TICK)
    {
        onTallyKill(playerIndex, st, actor, ++st->lastKills);
    }
    if (st->empowerFrames != 0 && !st->empowering)
    {
        st->empowerFrames--;
    }
    if (st->cleaveRow != NO_ROW)
    {
        skillExpansion_runtime[playerIndex].cooldowns[st->cleaveRow] = 0;
        st->cleaveRow = NO_ROW;
    }
    if (st->barukFrames != 0 && --st->barukFrames == 0)
    {
        st->barukCowed = 0;
    }
    if (st->holdFrames != 0)
    {
        actor->renderCoords.x += st->holdPos.x - actor->coords.x;
        actor->renderCoords.y += st->holdPos.y - actor->coords.y;
        actor->coords = st->holdPos;
        actor->velocity.x = actor->velocity.y = 0;
        if (--st->holdFrames == 0)
        {
            endHold(playerIndex, st, actor);
        }
    }
    if (st->countingFrames != 0 && --st->countingFrames == 0)
    {
        endCounting(playerIndex, st, actor);
    }
}

s32 skillExpansion_gimliDamage(u8 playerIndex, Actor *victim, s32 damage)
{
    GimliState *st;
    s32 grudgebearer = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_GRUDGEBEARER);
    s32 wager = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FRIENDLY_WAGER);
    s32 percent = 0;

    if (!isGimli(playerIndex) || victim == NULL || damage <= 0)
    {
        return damage;
    }
    st = state(playerIndex);
    if (st->grudgeType != NO_GRUDGE && victim->unitType == st->grudgeType)
    {
        percent += GRUDGE_PERCENT_PER_LEVEL * st->grudgeLevel +
                   GRUDGEBEARER_PERCENT_PER_LEVEL * grudgebearer;
        hackLib_fxBurst(victim, FX_SHAPE_SPARK, FX_COLOR_BLOOD, FX_LAYER_FRONT);
    }
    if (wager != 0 && st->wagerFrames != 0)
    {
        percent += WAGER_PERCENT_PER_LEVEL * wager;
    }
    if (st->empowerFrames != 0)
    {
        percent += EMPOWER_PERCENT;
    }
    return damage + percentOf(damage, percent);
}

/** Not Without Me: Gimli takes a share of a hit on his partner while near. */
static s32 guardPartner(u8 victim, s32 damage)
{
    u8 gimli = victim ^ 1;
    s32 level = passiveLevel(gimli, SKILL_EXPANSION_PASSIVE_NOT_WITHOUT_ME);
    Actor *guard = PLAYER(gimli).ownerActor;
    Actor *ward = PLAYER(victim).ownerActor;
    s32 share;

    if (!g_GameFlags.d.coopActive || !isGimli(gimli) || level == 0 || guard == NULL ||
        ward == NULL || PLAYER(gimli).currentHp <= 1 ||
        distanceSquared(guard, ward) > NOT_WITHOUT_ME_RADIUS * NOT_WITHOUT_ME_RADIUS)
    {
        return damage;
    }
    share = percentOf(damage, NOT_WITHOUT_ME_PERCENT_PER_LEVEL * level);
    if (share >= PLAYER(gimli).currentHp)
    {
        share = PLAYER(gimli).currentHp - 1;
    }
    if (share <= 0)
    {
        return damage;
    }
    skillExpansion_addHp(gimli, -share, FALSE);
    hackLib_fxBurst(guard, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
    return damage - share;
}

s32 skillExpansion_gimliIncomingDamage(u8 playerIndex, s32 damage)
{
    GimliState *st;
    Actor *actor;
    s32 mail;
    s32 percent = 0;

    if (damage <= 0 || playerIndex >= 2)
    {
        return damage;
    }
    if (!isGimli(playerIndex))
    {
        return guardPartner(playerIndex, damage);
    }
    st = state(playerIndex);
    actor = PLAYER(playerIndex).ownerActor;
    mail = passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DWARVEN_MAIL);
    if (st->holdFrames != 0)
    {
        u32 stored = st->holdStored + damage;

        st->holdStored = stored > HOLD_MAX_STORED ? HOLD_MAX_STORED : stored;
        damage = percentOf(damage, HOLD_TAKEN_PERCENT);
    }
    if (st->barukFrames != 0)
    {
        s32 guard = BARUK_PERCENT_PER_FOE * st->barukCowed;

        percent += guard > BARUK_MAX_PERCENT ? BARUK_MAX_PERCENT : guard;
    }
    if (mail != 0 && actor != NULL)
    {
        u32 foes = foesNear(actor, MAIL_RADIUS);

        percent += MAIL_PERCENT_PER_LEVEL * mail * (foes > MAIL_MAX_FOES ? MAIL_MAX_FOES : foes);
    }
    damage -= percentOf(damage, percent);
    if (damage < 1)
    {
        damage = 1;
    }
    if (st->raging && !st->durinUsed &&
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_DURINS_FOLK) != 0 &&
        damage >= PLAYER(playerIndex).currentHp)
    {
        st->durinUsed = TRUE;
        damage = PLAYER(playerIndex).currentHp - 1;
        if (actor != NULL)
        {
            castFx(actor, FX_SHAPE_RISING_STREAKS, FX_COLOR_GOLD, FX_LAYER_FRONT);
        }
        sfx_play(SFX_COMBAT_BLOCK);
        skillExpansion_feedback(playerIndex, "Durin's folk don't fall!");
    }
    return damage;
}

bool skillExpansion_gimliFlinch(u8 playerIndex, bool flinchOnDamage)
{
    if (!isGimli(playerIndex))
    {
        return flinchOnDamage;
    }
    if (state(playerIndex)->holdFrames != 0 ||
        passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_STUBBORN_AS_STONE) != 0)
    {
        return FALSE;
    }
    return flinchOnDamage;
}

void skillExpansion_gimliOnKill(u8 playerIndex, Actor *victim)
{
    GimliState *st;

    if (!isGimli(playerIndex) || victim == NULL)
    {
        return;
    }
    st = state(playerIndex);
    if (passiveLevel(playerIndex, SKILL_EXPANSION_PASSIVE_FRIENDLY_WAGER) != 0 &&
        st->wagerFrames == 0)
    {
        if (st->streakKills++ == 0)
        {
            st->streakFrames = WAGER_STREAK_FRAMES;
        }
        if (st->streakKills >= WAGER_STREAK_KILLS)
        {
            Actor *actor = PLAYER(playerIndex).ownerActor;

            st->streakKills = 0;
            st->streakFrames = 0;
            st->wagerFrames = WAGER_FRAMES;
            if (actor != NULL)
            {
                hackLib_fxTimed(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, WAGER_FRAMES,
                                FX_LAYER_FRONT);
            }
            sfx_play(SFX_DWARVEN_RAGE);
            skillExpansion_feedback(playerIndex, "Beat that, elf!");
        }
    }
    if (st->grudgeType != NO_GRUDGE && victim->unitType == st->grudgeType)
    {
        skillExpansion_gimliAddTally(playerIndex, 1);
    }
}

SkillExpansionCastResult skillExpansion_gimliBarukKhazad(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    GimliState *st = state(p);
    s32 stunFrames = BARUK_BASE_STUN_FRAMES + BARUK_STUN_FRAMES_PER_LEVEL * level;
    Actor *foe = NULL;
    u32 cowed = 0;

    while ((foe = findNextEnemy(playerActor, foe)) != NULL)
    {
        if (!skillExpansion_isFoe(foe) ||
            distanceSquared(playerActor, foe) > BARUK_RADIUS * BARUK_RADIUS)
        {
            continue;
        }
        hackLib_applyStun(foe, stunFrames);
        hackLib_fxBurst(foe, FX_SHAPE_SPARK, FX_COLOR_WHITE, FX_LAYER_FRONT);
        cowed++;
    }
    castFx(playerActor, FX_SHAPE_WIDE_RING, FX_COLOR_WHITE, FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
    sfx_play(SFX_DWARVEN_RAGE);
    skillExpansion_feedback(p, "Baruk Khazad!");
    if (cowed == 0)
    {
        return SKILL_CAST_SUCCESS;
    }
    st->barukCowed = cowed > 0xff ? 0xff : cowed;
    st->barukFrames = BARUK_BASE_FRAMES + BARUK_FRAMES_PER_LEVEL * level;
    hackLib_fxTimed(playerActor, FX_SHAPE_DIAMOND_RING, FX_COLOR_GHOST, st->barukFrames,
                    FX_LAYER_BEHIND);
    return SKILL_CAST_SUCCESS;
}

void skillExpansion_gimliHoldTheDeep(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    GimliState *st = state(p);

    st->holdLevel = castLevel(p);
    st->holdRow = PLAYER(p).castingActiveSkill;
    st->holdFrames = HOLD_FRAMES;
    st->holdStored = 0;
    st->holdPos = playerActor->coords;
    hackLib_fxTimed(playerActor, FX_SHAPE_SPARKLE_RING, FX_COLOR_WHITE, HOLD_FRAMES,
                    FX_LAYER_BEHIND);
    castFx(playerActor, FX_SHAPE_GROUND_POOL, FX_COLOR_GHOST, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_BLOCK);
    skillExpansion_feedback(p, "Let them come!");
}

SkillExpansionCastResult skillExpansion_gimliCleaveTheWeak(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    Actor *foe = nearestFoe(playerActor, CLEAVE_REACH);
    HackLibDamageResult result;
    s32 maxHp;
    bool execute;

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    maxHp = foe->unitType < ENEMY_TYPE_COUNT ? combat_getEnemyMaxHp(foe->unitType) : 0;
    execute =
        maxHp > 0 && foe->currentHp <= PLAYER_STAT(p, STAT_MAX_HP) &&
        foe->currentHp * 100 <= maxHp * (CLEAVE_BASE_PERCENT + CLEAVE_PERCENT_PER_LEVEL * level);
    result = skillExpansion_dealDamage(
        playerActor, foe,
        execute ? CLEAVE_EXECUTE_DAMAGE : CLEAVE_BASE_DAMAGE + CLEAVE_DAMAGE_PER_LEVEL * level,
        HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);
    hackLib_fxBurst(foe, execute ? FX_SHAPE_TENDRIL_STAR : FX_SHAPE_SPARK,
                    execute ? FX_COLOR_BLOOD : FX_COLOR_EMBER, FX_LAYER_FRONT);
    if (execute && result.killed)
    {
        // The cast paid before resolving and sets its cooldown after, so the reset waits a tick.
        player_addSpirit(p, skillExpansion_runtime[p].paidSpirit);
        skillExpansion_runtime[p].paidSpirit = 0;
        state(p)->cleaveRow = PLAYER(p).castingActiveSkill;
        sfx_play(SFX_COMBAT_INSTAKILL);
        skillExpansion_feedback(p, "Cleaved!");
    }
    else
    {
        sfx_play(SFX_COMBAT_CRIT_HIT);
    }
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gimliNobodyTossesADwarf(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    u8 level = castLevel(p);
    Actor *foe = nearestFoe(playerActor, TOSS_REACH);
    Vector2Fp16 preferred;
    Vector2Fp16 landing;
    s32 dx;
    s32 dy;
    s32 span;
    Actor *victim = NULL;
    u32 kills = 0;

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    dx = (foe->coords.x - playerActor->coords.x) >> FP16_16_SHIFT;
    dy = (foe->coords.y - playerActor->coords.y) >> FP16_16_SHIFT;
    span = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
    preferred = foe->coords;
    if (span > TOSS_LANDING_DISTANCE)
    {
        preferred.x -= (dx * TOSS_LANDING_DISTANCE / span) << FP16_16_SHIFT;
        preferred.y -= (dy * TOSS_LANDING_DISTANCE / span) << FP16_16_SHIFT;
    }
    if (!hackLib_findLanding(playerActor, &preferred, &landing))
    {
        return SKILL_CAST_BLOCKED_LANDING;
    }
    hackLib_fxAt(playerActor, &playerActor->coords, FX_SHAPE_SMOKE_SMALL, FX_COLOR_WHITE);
    playerActor->renderCoords.x += landing.x - playerActor->coords.x;
    playerActor->renderCoords.y += landing.y - playerActor->coords.y;
    playerActor->coords = landing;
    playerActor->velocity.x = playerActor->velocity.y = 0;
    castFx(playerActor, FX_SHAPE_SHOCKWAVE_ALT, FX_COLOR_GOLD, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_KNOCKDOWN);
    while ((victim = findNextEnemy(playerActor, victim)) != NULL)
    {
        HackLibDamageResult result;

        if (!skillExpansion_isFoe(victim) ||
            distanceSquared(playerActor, victim) > TOSS_RADIUS * TOSS_RADIUS)
        {
            continue;
        }
        result = skillExpansion_dealDamage(playerActor, victim,
                                           TOSS_BASE_DAMAGE + TOSS_DAMAGE_PER_LEVEL * level,
                                           HACKLIB_DAMAGE_ABILITY, PLAYER(p).castingActiveSkill);
        if (result.killed)
        {
            kills++;
        }
        else if (result.hit)
        {
            hackLib_applyStun(victim, TOSS_STUN_FRAMES);
        }
    }
    if (kills != 0)
    {
        skillExpansion_gimliAddTally(p, kills);
    }
    skillExpansion_feedback(p, "Nobody tosses a dwarf!");
    return SKILL_CAST_SUCCESS;
}

SkillExpansionCastResult skillExpansion_gimliBookOfGrudges(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    GimliState *st = state(p);
    Actor *foe = nearestFoe(playerActor, GRUDGE_REACH);

    if (foe == NULL)
    {
        return SKILL_CAST_INVALID_TARGET;
    }
    st->grudgeType = foe->unitType;
    st->grudgeLevel = castLevel(p);
    hackLib_fxPace(hackLib_fxBurst(foe, FX_SHAPE_TENDRIL_BURST, FX_COLOR_BLOOD, FX_LAYER_FRONT),
                   CAST_FX_TICKS);
    hackLib_fxBurst(foe, FX_SHAPE_OVAL_RING, FX_COLOR_EMBER, FX_LAYER_BEHIND);
    sfx_play(SFX_COMBAT_CRIT_HIT);
    skillExpansion_feedback(p, "It goes in the Book!");
    return SKILL_CAST_SUCCESS;
}

void skillExpansion_gimliStillCounting(Actor *playerActor)
{
    u8 p = playerActor->as.combat.playerIndex;
    GimliState *st = state(p);

    st->countingFrames = COUNTING_BASE_FRAMES + COUNTING_FRAMES_PER_LEVEL * castLevel(p);
    st->countingKills = 0;
    st->countingRow = PLAYER(p).castingActiveSkill;
    hackLib_fxTimed(playerActor, FX_SHAPE_RISING_STREAKS, FX_COLOR_BLOOD, st->countingFrames,
                    FX_LAYER_FRONT);
    castFx(playerActor, FX_SHAPE_TENDRIL_STAR, FX_COLOR_NIGHT, FX_LAYER_FRONT);
    sfx_play(SFX_DWARVEN_RAGE);
    skillExpansion_feedback(p, "Still counting!");
}

void skillExpansion_gimliCastStarted(u8 playerIndex)
{
    GimliState *st;

    if (!isGimli(playerIndex))
    {
        return;
    }
    st = state(playerIndex);
    if (st->chargeKills >= CHARGE_KILLS)
    {
        st->empowering = TRUE;
        st->empowerFrames = EMPOWER_FRAMES;
    }
}

void skillExpansion_gimliCastEnded(u8 playerIndex, bool success)
{
    GimliState *st;
    Actor *actor;

    if (!isGimli(playerIndex) || !state(playerIndex)->empowering)
    {
        return;
    }
    st = state(playerIndex);
    st->empowering = FALSE;
    if (!success)
    {
        st->empowerFrames = 0;
        return;
    }
    st->chargeKills = 0;
    player_addSpirit(playerIndex, skillExpansion_runtime[playerIndex].paidSpirit);
    actor = PLAYER(playerIndex).ownerActor;
    if (actor != NULL)
    {
        hackLib_fxTimed(actor, FX_SHAPE_SPARKLE_RING, FX_COLOR_GOLD, EMPOWER_FRAMES,
                        FX_LAYER_FRONT);
        castFx(actor, FX_SHAPE_STAR_BLOOM, FX_COLOR_GOLD, FX_LAYER_FRONT);
    }
    skillExpansion_feedback(playerIndex, "Empowered!");
}

bool32 skillExpansion_canCastGimliHold(u8 playerIndex)
{
    return state(playerIndex)->holdFrames == 0;
}

bool32 skillExpansion_canCastGimliCounting(u8 playerIndex)
{
    return state(playerIndex)->countingFrames == 0;
}

static const HackLibHudMeterLook BarCharge = {HACKLIB_HUD_COLOR_PALE_GOLD, HACKLIB_HUD_COLOR_GOLD,
                                              HACKLIB_HUD_COLOR_DARK_GOLD, HACKLIB_HUD_COLOR_WHITE};
static const HackLibHudMeterLook BarCharged = {HACKLIB_HUD_COLOR_WHITE, HACKLIB_HUD_COLOR_PALE_GOLD,
                                               HACKLIB_HUD_COLOR_GOLD, HACKLIB_HUD_COLOR_WHITE};
static const u8 ChargeBarMarks[] = {CHARGE_KILLS / 2};

void skillExpansion_gimliDrawTallyBar(u8 playerIndex)
{
    const HackLibHudMeterLook *look = &BarCharge;

    if (!isGimli(playerIndex))
    {
        return;
    }
    GimliState *st = state(playerIndex);
    if (st->chargeKills >= CHARGE_KILLS)
    {
        if (++st->barPulseFrames >= 2 * BAR_PULSE_FRAMES)
        {
            st->barPulseFrames = 0;
        }
        if (st->barPulseFrames < BAR_PULSE_FRAMES)
        {
            look = &BarCharged;
        }
    }
    hackLib_drawHudMeter(&HackLibHudGapMeter, st->chargeKills, CHARGE_KILLS, look, ChargeBarMarks,
                         ARRAY_COUNT(ChargeBarMarks));
}
