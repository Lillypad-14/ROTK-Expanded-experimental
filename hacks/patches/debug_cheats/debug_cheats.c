#include "patches/debug_cheats/debug_cheats.h"
#include "patches/mod_save/save_sram.h"
#include "player.h"
#include "stats.h"
#include "variables.h"
#if HACK_gauntlet
#include "patches/gauntlet/gauntlet.h"
#endif
#if HACK_skill_expansion
#include "patches/skill_expansion/skill_expansion_gimli.h"
#endif

#define DEBUG_CHEATS_MAX_LEVEL 50
#define DEBUG_CHEATS_POINTS 99
#define ATTRIBUTE_POINTS_PER_LEVEL 5
// Zero-based, matching the skill_expansion level-up hook.
#define DOUBLE_SKILL_POINTS_LEVEL 20
// Enough for every Gimli page 2 rank, below the 2000 kills that unlock Moria.
#define DEBUG_CHEATS_KILLS 30000

struct DebugCheatsState g_DebugCheatsState;

void debugCheats_loadState(void)
{
    GLOBAL_MOD_OPTIONS_READ(debugCheatsState, &g_DebugCheatsState);
    if (g_DebugCheatsState.tag != DEBUG_CHEATS_TAG)
    {
        g_DebugCheatsState = (struct DebugCheatsState){.tag = DEBUG_CHEATS_TAG};
    }
}

static u8 addPoints(u8 points, s32 amount)
{
    s32 total = points + amount;

    return total > 0xff ? 0xff : total;
}

static void showUnspentPoints(u8 playerIndex)
{
    PLAYER(playerIndex).statusFlags.p |= PLAYER_STATUS_FLAG_UNSPENT_POINTS;
    if (g_PlayerHuds[playerIndex].skillPanelState == PLAYERHUD_SKILL_PANEL_IDLE)
    {
        g_PlayerHuds[playerIndex].skillPanelState = PLAYERHUD_SKILL_PANEL_DRAW_UNSPENT_POINTS;
    }
}

void debugCheats_playerTick(u8 playerIndex)
{
    Player *player = &PLAYER(playerIndex);
    bool32 progressionCheats = TRUE;
#if HACK_gauntlet
    progressionCheats = !gauntlet_isRunning();
#endif

    if (progressionCheats && g_DebugCheatsState.maxLevel && player->level < DEBUG_CHEATS_MAX_LEVEL)
    {
        s32 levels = DEBUG_CHEATS_MAX_LEVEL - player->level;
        s32 skillPoints = levels;

#if HACK_skill_expansion
        s32 firstDoublePointLevel = player->level + 1;

        if (firstDoublePointLevel < DOUBLE_SKILL_POINTS_LEVEL)
        {
            firstDoublePointLevel = DOUBLE_SKILL_POINTS_LEVEL;
        }
        skillPoints += DEBUG_CHEATS_MAX_LEVEL - firstDoublePointLevel + 1;
#endif

        player->level = DEBUG_CHEATS_MAX_LEVEL;
        player->experience = 0;
        player->attributePoints =
            addPoints(player->attributePoints, ATTRIBUTE_POINTS_PER_LEVEL * levels);
        player->skillPoints = addPoints(player->skillPoints, skillPoints);
        g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_XP_DIRTY;
        showUnspentPoints(playerIndex);
    }
    if (progressionCheats && g_DebugCheatsState.skillPoints &&
        player->skillPoints < DEBUG_CHEATS_POINTS)
    {
        player->skillPoints = DEBUG_CHEATS_POINTS;
        showUnspentPoints(playerIndex);
    }
    if (progressionCheats && g_DebugCheatsState.attributePoints &&
        player->attributePoints < DEBUG_CHEATS_POINTS)
    {
        player->attributePoints = DEBUG_CHEATS_POINTS;
        showUnspentPoints(playerIndex);
    }
    if (g_DebugCheatsState.godMode && player->currentHp > 0 &&
        player->currentHp < PLAYER_STAT(playerIndex, STAT_MAX_HP))
    {
        player->currentHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    }
    if (g_DebugCheatsState.infiniteSpirit &&
        player->currentSpirit < PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT))
    {
        player->currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    }
#if HACK_skill_expansion
    if (progressionCheats && g_DebugCheatsState.maxKills)
    {
        skillExpansion_gimliRaiseTally(playerIndex, DEBUG_CHEATS_KILLS);
    }
#endif
}

s32 debugCheats_meleeDamage(Actor *victim, s32 damage)
{
    if (g_DebugCheatsState.oneHitKills && victim->currentHp > damage)
    {
        return victim->currentHp;
    }
    return damage;
}

#if HACK_mod_options
static const char *const DebugCheatsOffOnNames[2] = {"Off", "On"};

static void writeState(void)
{
    GLOBAL_MOD_OPTIONS_WRITE(debugCheatsState, &g_DebugCheatsState);
}

#define DEBUG_CHEATS_TOGGLE(field)                                                                 \
    static u8 get_##field(void)                                                                    \
    {                                                                                              \
        return g_DebugCheatsState.field;                                                           \
    }                                                                                              \
    static void set_##field(u8 value)                                                              \
    {                                                                                              \
        g_DebugCheatsState.field = value;                                                          \
        writeState();                                                                              \
    }

DEBUG_CHEATS_TOGGLE(maxLevel)
DEBUG_CHEATS_TOGGLE(skillPoints)
DEBUG_CHEATS_TOGGLE(attributePoints)
DEBUG_CHEATS_TOGGLE(godMode)
DEBUG_CHEATS_TOGGLE(infiniteSpirit)
DEBUG_CHEATS_TOGGLE(oneHitKills)
DEBUG_CHEATS_TOGGLE(maxKills)

const struct ModOptionEntry DebugCheatsOptions[DEBUG_CHEATS_OPTION_COUNT] = {
    {"Max Level", DebugCheatsOffOnNames, 2, 0, get_maxLevel, set_maxLevel},
    {"Infinite Skill Pts", DebugCheatsOffOnNames, 2, 0, get_skillPoints, set_skillPoints},
    {"Infinite Attr. Pts", DebugCheatsOffOnNames, 2, 0, get_attributePoints, set_attributePoints},
    {"God Mode", DebugCheatsOffOnNames, 2, 0, get_godMode, set_godMode},
    {"Infinite Spirit", DebugCheatsOffOnNames, 2, 0, get_infiniteSpirit, set_infiniteSpirit},
    {"One-Hit Kills", DebugCheatsOffOnNames, 2, 0, get_oneHitKills, set_oneHitKills},
    {"Max Kills", DebugCheatsOffOnNames, 2, 0, get_maxKills, set_maxKills},
};
#endif
