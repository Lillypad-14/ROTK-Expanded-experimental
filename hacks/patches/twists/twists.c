#include "patches/twists/twists.h"
#include "memory.h"
#include "patches/mod_save/save_sram.h"
#include "patches/skill_expansion/skill_expansion_runtime.h"
#include "player.h"
#include "rng.h"
#include "stats.h"
#include "variables.h"
#if HACK_gauntlet
#include "patches/gauntlet/gauntlet.h"
#endif

#define TWISTS_TAG 0x54 // a blank (0xFF) or foreign area never reads as written
#define TWISTS_FRAMES_PER_SECOND 60
// Timers count in quarter-second ticks so they fit a byte.
#define TWISTS_TICK_MASK 3
#define TWISTS_STREAK_TICKS (5 * TWISTS_FRAMES_PER_SECOND / 4)
#define TWISTS_WOUND_TICKS (5 * TWISTS_FRAMES_PER_SECOND / 4)
#define TWISTS_SURGE_TICKS (10 * TWISTS_FRAMES_PER_SECOND / 4)
#define TWISTS_MOMENTUM_PERCENT_PER_KILL 5
#define TWISTS_MOMENTUM_MAX_KILLS 10
#define TWISTS_PERCENT_FRAME_UNIT (100 * TWISTS_FRAMES_PER_SECOND)

// Option indexes whose table entry is the neutral setting.
#define TWISTS_DEALT_NEUTRAL 2
#define TWISTS_TAKEN_NEUTRAL 3
#define TWISTS_XP_NEUTRAL 1

#define TWISTS_WHISPER_HALF_KILLS 5
#define TWISTS_WHISPER_FULL_KILLS 10
#define TWISTS_STREAK_MAX 15
#define TWISTS_SURGE_PERCENT 50
#define TWISTS_MIN_TAKEN_PERCENT 10

#define TWISTS_FELLOWSHIP_NEAR_PIXELS 80
#define TWISTS_FELLOWSHIP_FAR_PIXELS 180
#define TWISTS_FELLOWSHIP_PERCENT 15

// g_Fate: low nibble is the level's omen, the next bits mark a banner still to show per player.
#define TWISTS_FATE_OMEN_MASK 0x0f
#define TWISTS_FATE_ANNOUNCE(playerIndex) (0x10 << (playerIndex))

enum TwistsOmen
{
    OMEN_NONE,
    OMEN_BLOOD_MOON,
    OMEN_FORTUNE,
    OMEN_VAMPIRE_NIGHT,
    OMEN_BOUNTY,
    OMEN_HEARTH,
    OMEN_IRON_SKIN,
    OMEN_HUNT,
    OMEN_COUNT
};

/** Persisted settings, one byte each. The struct is the save area's size. */
struct TwistsState
{
    u8 tag;
    u8 damageDealt;
    u8 damageTaken;
    u8 lifeDrain;
    u8 luckyStrikes;
    u8 berserker;
    u8 momentum;
    u8 hpRegen;
    u8 spiritRegen;
    u8 killHeal;
    u8 killSpirit;
    u8 partyShare;
    u8 woundLock;
    u8 xpGain;
    u8 omens;
    u8 ringWhisper;
    u8 fellowship; // only the value 1 is on, an older save leaves this byte blank
};

typedef struct TwistsRuntime
{
    u16 hpRegenProgress;
    u16 spiritRegenProgress;
    u8 streakTicks;
    u8 woundTicks;
    u8 surgeTicks;
    u8 streak : 4;
    u8 ringKills : 4;
} TwistsRuntime;

static const u16 DealtPercents[] = {50, 75, 100, 125, 150, 200, 300};
static const u16 TakenPercents[] = {25, 50, 75, 100, 150, 200, 300};
static const u8 LifeDrainPercents[] = {0, 5, 10, 20, 35};
static const u8 LuckyPercents[] = {0, 5, 10, 20, 33};
static const u8 HpRegenPercents[] = {0, 1, 2, 4};
static const u8 SpiritRegenPercents[] = {0, 2, 4, 8};
static const u8 KillHealPercents[] = {0, 3, 6, 12, 25};
static const u8 KillSpiritPercents[] = {0, 5, 10, 20, 40};
static const u16 XpPercents[] = {50, 100, 150, 200, 300, 500};

static const char *const OmenBanners[OMEN_COUNT] = {
    NULL,
    "Omen: Blood Moon +50%/+50%",
    "Omen: Fortune, lucky strikes",
    "Omen: Vampire Night, drain",
    "Omen: Bounty, double XP",
    "Omen: Hearth, HP regen",
    "Omen: Iron Skin, -35% hurt",
    "Omen: The Hunt, momentum",
};

static struct TwistsState g_Twists;
static u8 g_Fate;
static TwistsRuntime runtimes[2];

void twists_loadState(void)
{
    GLOBAL_MOD_OPTIONS_READ(twistsState, &g_Twists);
    if (g_Twists.tag != TWISTS_TAG)
    {
        g_Twists = (struct TwistsState){.tag = TWISTS_TAG,
                                        .damageDealt = TWISTS_DEALT_NEUTRAL,
                                        .damageTaken = TWISTS_TAKEN_NEUTRAL,
                                        .xpGain = TWISTS_XP_NEUTRAL};
    }
}

static s32 percentOf(s32 value, s32 percent)
{
    s32 result = value * percent / 100;

    return result < 1 && percent > 0 ? 1 : result;
}

static u32 currentOmen(void)
{
    return g_Twists.omens ? g_Fate & TWISTS_FATE_OMEN_MASK : OMEN_NONE;
}

void twists_missionBegin(u8 playerIndex)
{
    memory_memset(&runtimes[playerIndex], 0, sizeof(TwistsRuntime));
    if (playerIndex == 0)
    {
        g_Fate = g_Twists.omens ? rng_rollRange(OMEN_BLOOD_MOON, OMEN_COUNT - 1) : OMEN_NONE;
    }
    g_Fate |= TWISTS_FATE_ANNOUNCE(playerIndex);
}

/** Percent bonus (near) or penalty (far) to damage from standing close to the other player. */
static s32 fellowshipPercent(u8 playerIndex)
{
    Actor *self = PLAYER(playerIndex).ownerActor;
    Actor *other = PLAYER(playerIndex ^ 1).ownerActor;
    s32 dx;
    s32 dy;
    s32 distanceSquared;

    if (g_Twists.fellowship != 1 || g_PlayerGlobals.activePlayerCount < 2 || self == NULL ||
        other == NULL || PLAYER(playerIndex ^ 1).currentHp <= 0)
    {
        return 0;
    }
    dx = (self->coords.x - other->coords.x) >> FP16_16_SHIFT;
    dy = (self->coords.y - other->coords.y) >> FP16_16_SHIFT;
    distanceSquared = dx * dx + dy * dy;
    if (distanceSquared <= TWISTS_FELLOWSHIP_NEAR_PIXELS * TWISTS_FELLOWSHIP_NEAR_PIXELS)
    {
        return TWISTS_FELLOWSHIP_PERCENT;
    }
    return distanceSquared >= TWISTS_FELLOWSHIP_FAR_PIXELS * TWISTS_FELLOWSHIP_FAR_PIXELS
               ? -TWISTS_FELLOWSHIP_PERCENT
               : 0;
}

static void healHp(u8 playerIndex, s32 amount)
{
    Player *player = &PLAYER(playerIndex);
    s32 maxHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    s32 hp = player->currentHp;

    if (hp <= 0 || amount <= 0)
    {
        return;
    }
    hp += amount;
    player->currentHp = hp > maxHp ? maxHp : hp;
}

static void healSpirit(u8 playerIndex, s32 amount)
{
    Player *player = &PLAYER(playerIndex);
    s32 maxSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    s32 spirit = player->currentSpirit + amount;

    if (amount > 0)
    {
        player->currentSpirit = spirit > maxSpirit ? maxSpirit : spirit;
    }
}

/** Add percent of max per second at 60 fps, returning the whole points earned. */
static s32 regenPoints(u16 *progress, s32 max, s32 percent)
{
    s32 points;

    *progress += max * percent;
    points = *progress / TWISTS_PERCENT_FRAME_UNIT;
    *progress -= points * TWISTS_PERCENT_FRAME_UNIT;
    return points;
}

void twists_playerTick(u8 playerIndex)
{
    TwistsRuntime *runtime = &runtimes[playerIndex];
    Player *player = &PLAYER(playerIndex);
    s32 hpRegen = HpRegenPercents[g_Twists.hpRegen] + (currentOmen() == OMEN_HEARTH ? 2 : 0);

    if (g_Fate & TWISTS_FATE_ANNOUNCE(playerIndex))
    {
        g_Fate &= ~TWISTS_FATE_ANNOUNCE(playerIndex);
        if (currentOmen() != OMEN_NONE)
        {
            skillExpansion_feedback(playerIndex, OmenBanners[currentOmen()]);
        }
    }
    if ((g_FrameCounter & TWISTS_TICK_MASK) == 0)
    {
        if (runtime->streakTicks != 0 && --runtime->streakTicks == 0)
        {
            runtime->streak = 0;
        }
        if (runtime->woundTicks != 0)
        {
            runtime->woundTicks--;
        }
        if (runtime->surgeTicks != 0 && --runtime->surgeTicks == 0)
        {
            skillExpansion_feedback(playerIndex, "The whisper fades");
        }
    }
    if (player->currentHp <= 0)
    {
        return;
    }
    if (hpRegen != 0 && runtime->woundTicks == 0)
    {
        healHp(playerIndex, regenPoints(&runtime->hpRegenProgress,
                                        PLAYER_STAT(playerIndex, STAT_MAX_HP), hpRegen));
    }
    if (g_Twists.spiritRegen != 0)
    {
        healSpirit(playerIndex, regenPoints(&runtime->spiritRegenProgress,
                                            PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT),
                                            SpiritRegenPercents[g_Twists.spiritRegen]));
    }
}

asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global twists_original_awardXp\n"
    ".thumb_func\n"
    "twists_original_awardXp:\n"
    "    push {r4-r7, lr}\n"
    "    mov r7, r10\n"
    "    mov r6, r9\n"
    "    mov r5, r8\n"
    "    ldr r3, =player_awardXp + 8\n"
    "    bx r3\n"
    "    .pool\n");

void twists_original_awardXp(s16 xp);

/**
 * Scale an XP award.
 *
 * WRAPS: player_awardXp.
 */
void twists_awardXp(s16 xp)
{
    s32 percent = XpPercents[g_Twists.xpGain] + (currentOmen() == OMEN_BOUNTY ? 100 : 0);
    s32 scaled;

#if HACK_gauntlet
    percent = percent * gauntlet_xpPercent() / 100;
#endif
    scaled = xp * percent / 100;

    twists_original_awardXp(scaled > 0x7fff ? 0x7fff : scaled);
}

s32 twists_damageDealt(u8 playerIndex, s32 damage)
{
    s32 percent = DealtPercents[g_Twists.damageDealt];
    s32 maxHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    s32 hp = PLAYER(playerIndex).currentHp;
    s32 luckyPercent = LuckyPercents[g_Twists.luckyStrikes];

    if (currentOmen() == OMEN_BLOOD_MOON)
    {
        percent += 50;
    }
    percent += fellowshipPercent(playerIndex);
    if (runtimes[playerIndex].surgeTicks != 0)
    {
        percent += TWISTS_SURGE_PERCENT;
    }
    if (currentOmen() == OMEN_FORTUNE && luckyPercent < 25)
    {
        luckyPercent = 25;
    }
    if (g_Twists.berserker && maxHp > 0 && hp > 0 && hp < maxHp)
    {
        percent += 100 * (maxHp - hp) / maxHp;
    }
    if (g_Twists.momentum || currentOmen() == OMEN_HUNT)
    {
        u32 kills = runtimes[playerIndex].streak;

        percent += TWISTS_MOMENTUM_PERCENT_PER_KILL *
                   (kills > TWISTS_MOMENTUM_MAX_KILLS ? TWISTS_MOMENTUM_MAX_KILLS : kills);
    }
    damage = percentOf(damage, percent);
    if (luckyPercent != 0 && rng_rollRange(1, 100) <= luckyPercent)
    {
        damage *= 2;
    }
    return damage;
}

void twists_enemyHitResolved(u8 playerIndex, s32 hpBefore)
{
    Player *player = &PLAYER(playerIndex);
    TwistsRuntime *runtime = &runtimes[playerIndex];
    s32 lost = hpBefore - player->currentHp;
    s32 percent = TakenPercents[g_Twists.damageTaken];
    s32 scaled;

    if (lost <= 0)
    {
        return;
    }
    runtime->streak = 0;
    if (g_Twists.woundLock)
    {
        runtime->woundTicks = TWISTS_WOUND_TICKS;
    }
    if (player->currentHp <= 0)
    {
        return;
    }
    if (currentOmen() == OMEN_BLOOD_MOON)
    {
        percent += 50;
    }
    else if (currentOmen() == OMEN_IRON_SKIN)
    {
        percent -= 35;
    }
    if (fellowshipPercent(playerIndex) > 0)
    {
        percent -= TWISTS_FELLOWSHIP_PERCENT;
    }
    if (runtime->surgeTicks != 0)
    {
        percent += TWISTS_SURGE_PERCENT;
    }
    scaled = percentOf(lost, percent < TWISTS_MIN_TAKEN_PERCENT ? TWISTS_MIN_TAKEN_PERCENT : percent);
    if (scaled < lost)
    {
        healHp(playerIndex, lost - scaled);
    }
    else if (scaled > lost)
    {
        // Extra damage never kills on its own.
        s32 room = player->currentHp - 1;

        player->currentHp -= scaled - lost < room ? scaled - lost : room;
    }
}

static void rewardKill(u8 playerIndex)
{
    healHp(playerIndex, percentOf(PLAYER_STAT(playerIndex, STAT_MAX_HP),
                                  KillHealPercents[g_Twists.killHeal]));
    healSpirit(playerIndex, percentOf(PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT),
                                      KillSpiritPercents[g_Twists.killSpirit]));
}

void twists_combatEvent(u8 playerIndex, const HackLibCombatEvent *event)
{
    TwistsRuntime *runtime = &runtimes[playerIndex];

    if (!event->result.hit || event->result.damageDealt <= 0)
    {
        return;
    }
    healHp(playerIndex,
           percentOf(event->result.damageDealt, LifeDrainPercents[g_Twists.lifeDrain] +
                                                    (currentOmen() == OMEN_VAMPIRE_NIGHT ? 15 : 0)));
    if (!event->result.killed)
    {
        return;
    }
    if (runtime->streak < TWISTS_STREAK_MAX)
    {
        runtime->streak++;
    }
    runtime->streakTicks = TWISTS_STREAK_TICKS;
    rewardKill(playerIndex);
    if (g_Twists.ringWhisper && runtime->surgeTicks == 0)
    {
        runtime->ringKills++;
        if (runtime->ringKills >= TWISTS_WHISPER_FULL_KILLS)
        {
            runtime->ringKills = 0;
            runtime->surgeTicks = TWISTS_SURGE_TICKS;
            skillExpansion_feedback(playerIndex, "The Ring calls! +50%/+50%");
        }
        else if (runtime->ringKills == TWISTS_WHISPER_HALF_KILLS)
        {
            skillExpansion_feedback(playerIndex, "The Ring stirs...");
        }
    }
    if (g_Twists.partyShare)
    {
        for (u8 other = 0; other < g_PlayerGlobals.activePlayerCount && other < 2; other++)
        {
            if (other != playerIndex)
            {
                rewardKill(other);
            }
        }
    }
}

#if HACK_mod_options
static const char *const OffOnNames[2] = {"Off", "On"};
static const char *const DealtNames[7] = {"50%", "75%", "100%", "125%", "150%", "200%", "300%"};
static const char *const TakenNames[7] = {"25%", "50%", "75%", "100%", "150%", "200%", "300%"};
static const char *const DrainNames[5] = {"Off", "5%", "10%", "20%", "35%"};
static const char *const LuckyNames[5] = {"Off", "5%", "10%", "20%", "33%"};
static const char *const HpRegenNames[4] = {"Off", "1%/s", "2%/s", "4%/s"};
static const char *const SpiritRegenNames[4] = {"Off", "2%/s", "4%/s", "8%/s"};
static const char *const KillHealNames[5] = {"Off", "3%", "6%", "12%", "25%"};
static const char *const KillSpiritNames[5] = {"Off", "5%", "10%", "20%", "40%"};
static const char *const XpNames[6] = {"50%", "100%", "150%", "200%", "300%", "500%"};

static void writeState(void)
{
    GLOBAL_MOD_OPTIONS_WRITE(twistsState, &g_Twists);
}

#define TWISTS_SETTING(field)                                                                      \
    static u8 get_##field(void)                                                                    \
    {                                                                                              \
        return g_Twists.field;                                                                     \
    }                                                                                              \
    static void set_##field(u8 value)                                                              \
    {                                                                                              \
        g_Twists.field = value;                                                                    \
        writeState();                                                                              \
    }

TWISTS_SETTING(damageDealt)
TWISTS_SETTING(damageTaken)
TWISTS_SETTING(lifeDrain)
TWISTS_SETTING(luckyStrikes)
TWISTS_SETTING(berserker)
TWISTS_SETTING(momentum)
TWISTS_SETTING(hpRegen)
TWISTS_SETTING(spiritRegen)
TWISTS_SETTING(killHeal)
TWISTS_SETTING(killSpirit)
TWISTS_SETTING(partyShare)
TWISTS_SETTING(woundLock)
TWISTS_SETTING(xpGain)
TWISTS_SETTING(omens)
TWISTS_SETTING(ringWhisper)
TWISTS_SETTING(fellowship)

const struct ModOptionEntry TwistsCombatOptions[TWISTS_COMBAT_OPTION_COUNT] = {
    {"Damage Dealt", DealtNames, 7, 0, get_damageDealt, set_damageDealt,
     "Scales all damage you deal.\nStacks with other twists."},
    {"Damage Taken", TakenNames, 7, 0, get_damageTaken, set_damageTaken,
     "Scales enemy attack damage.\nExtra damage never kills."},
    {"Life Drain", DrainNames, 5, 0, get_lifeDrain, set_lifeDrain,
     "Heal a share of the damage\nyou deal on each hit."},
    {"Lucky Strikes", LuckyNames, 5, 0, get_luckyStrikes, set_luckyStrikes,
     "Chance for a hit to deal\ndouble damage."},
    {"Berserker", OffOnNames, 2, 0, get_berserker, set_berserker,
     "Deal more damage as your\nHP falls, up to +100%."},
    {"Momentum", OffOnNames, 2, 0, get_momentum, set_momentum,
     "+5% damage per kill, max 10.\nLost on a hit or after 5s."},
};

const struct ModOptionEntry TwistsVigorOptions[TWISTS_VIGOR_OPTION_COUNT] = {
    {"HP Regen", HpRegenNames, 4, 0, get_hpRegen, set_hpRegen,
     "Regain HP over time.\nPaused by Wound Lock."},
    {"Spirit Regen", SpiritRegenNames, 4, 0, get_spiritRegen, set_spiritRegen,
     "Regain Spirit over time."},
    {"Kill: Heal", KillHealNames, 5, 0, get_killHeal, set_killHeal,
     "Each kill restores a share\nof your max HP."},
    {"Kill: Spirit", KillSpiritNames, 5, 0, get_killSpirit, set_killSpirit,
     "Each kill restores a share\nof your max Spirit."},
    {"Party Share", OffOnNames, 2, 0, get_partyShare, set_partyShare,
     "Kill rewards also go to\nyour partner."},
    {"Wound Lock", OffOnNames, 2, 0, get_woundLock, set_woundLock,
     "Taking a hit stops HP\nregen for 5 seconds."},
};

const struct ModOptionEntry TwistsFateOptions[TWISTS_FATE_OPTION_COUNT] = {
    {"XP Gain", XpNames, 6, 0, get_xpGain, set_xpGain,
     "Scales all XP you earn,\nfrom kills and rewards."},
    {"Omens", OffOnNames, 2, 0, get_omens, set_omens,
     "Each level rolls a random\nomen with a bonus or curse."},
    {"Ring's Whisper", OffOnNames, 2, 0, get_ringWhisper, set_ringWhisper,
     "10 kills fill the Ring: 10s\nof +50% dealt and taken."},
    {"Fellowship", OffOnNames, 2, 0, get_fellowship, set_fellowship,
     "Co-op: near ally +15% dmg,\n-15% hurt. Far: -15% dmg."},
};
#endif
