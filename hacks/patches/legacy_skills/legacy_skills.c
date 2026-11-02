#if !HACK_skill_expansion || !HACK_mod_save
#error "legacy_skills requires skill_expansion and mod_save"
#endif

#include "actor.h"
#include "combat.h"
#include "fx.h"
#include "memory.h"
#include "patches/legacy_skills/legacy_fx.h"
#include "patches/legacy_skills/legacy_skills.h"
#include "patches/mod_save/save_layout.h"
#include "patches/mod_save/save_slots.h"
#include "patches/mod_save/save_sram.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "lib/pc_host.h"
#include "player.h"
#include "rng.h"
#include "sfx.h"
#include "spriteAnimation.h"
#include "scene.h"
#include "scene/saveLoadMenu.h"
#include "scene/skillTree.h"
#include "skill.h"
#include "stats.h"
#include "text.h"
#include "variables.h"

#define LEGACY_SKILL_RECORD_TAG 0x01
#define LEGACY_SKILL_COUNT 11
#define LEGACY_PASSIVE_STATS 2
#define LEGACY_HERO_COUNT 6
#define LEGACY_PASSIVE_MAX 6
#define LEGACY_SKILL_LEVEL_MAX 20
#define LEGACY_SKILL_RECORD_EMPTY 0xff

// One stat a legacy passive adds: base + perLevel * level / divisor at levels 1 and up.
typedef struct LegacyPassiveStat
{
    u8 stat;
    s8 base;
    s8 perLevel;
    u8 divisor;
} LegacyPassiveStat;

// A hero's legacy passives, in the skill record's order (they come first).
typedef struct LegacyHeroPassives
{
    u8 count;
    LegacyPassiveStat stats[LEGACY_PASSIVE_MAX][LEGACY_PASSIVE_STATS];
} LegacyHeroPassives;

#define NONE {STAT_NONE, 0, 0, 1}
#define PER_LEVEL(stat, perLevel) {stat, 0, perLevel, 1}
// value at level 1 is `first`, each further level adds `step`
#define FROM(stat, first, step) {stat, (first) - (step), step, 1}
// base + perLevel * level, The Two Towers' own form
#define BASE_PLUS(stat, base, perLevel) {stat, base, perLevel, 1}

// The Two Towers passives by hero id, as each one's code in The Two Towers adds them (the cast table
// entry, 0x08057AB0), which differs from several descriptions. A skill with no ROTK mechanic uses
// the closest stat.
static const LegacyHeroPassives HeroPassives[LEGACY_HERO_COUNT] = {
    // Frodo: Resist One Ring, Strength of the Shire, Rummage, Sneak Attack, Duck
    {5,
     {{PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 4), NONE},
      {PER_LEVEL(STAT_HP_REGEN, 1), {STAT_SPIRIT_REGEN, 0, 1, 3}},
      {BASE_PLUS(STAT_EXTRA_TREASURE_PERCENT, 5, 1), NONE},
      {PER_LEVEL(STAT_BACKSTAB, 2), NONE},
      {PER_LEVEL(STAT_MELEE_ARMOR, 1), NONE}}},
    // Legolas: Meditation, Fleet of Foot, Orc Slayer, Hawk Eye, Critical Shot
    {5,
     {{PER_LEVEL(STAT_MEDITATIVE, 1), NONE},
      {PER_LEVEL(STAT_SPEED_PERCENT, 3), NONE},
      {PER_LEVEL(STAT_DAMAGE_TO_ORCS, 2), NONE},
      {PER_LEVEL(STAT_DAMAGE_WITH_BOW, 1), NONE},
      {BASE_PLUS(STAT_CRIT_CHANCE_PERCENT, 10, 1), NONE}}},
    // Aragorn: Dual Wield, Numenorean Blood, Shield Offense, Parry, Ferocity
    {5,
     {{PER_LEVEL(STAT_ARAGORN_DUAL_WIELD_DAMAGE, 1), NONE},
      {PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 5), PER_LEVEL(STAT_DAMAGE_TO_NAZGUL, 2)},
      {{STAT_DAMAGE_WITH_SHIELD, 0, 1, 2}, PER_LEVEL(STAT_BLOCK_PERCENT, 1)},
      {FROM(STAT_PARRY_PERCENT, 5, 1), NONE},
      {BASE_PLUS(STAT_HP_PERCENT_PER_KILL, 5, 1), NONE}}},
    // Gandalf: Elusion, Inspire, Wisdom, Flame of Anor (Two Towers 0x0803AA20, 0x0803A928,
    // 0x0803AAB4, 0x0803A98C)
    {4,
     {{BASE_PLUS(STAT_CORRUPTION_RESIST_PERCENT, 20, 4), NONE},
      {PER_LEVEL(STAT_SPIRIT_REGEN_PERCENT, 25), NONE},
      {PER_LEVEL(STAT_WISDOM, 1), NONE},
      {BASE_PLUS(STAT_REVIVE_CHANCE_PERCENT, 20, 2), NONE}}},
    // Eowyn: Strength of Heart, Iron Will, Stand Fast, Eyes of Rohan, Ferocity
    {5,
     {{PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 5), PER_LEVEL(STAT_DAMAGE_TO_NAZGUL, 2)},
      {PER_LEVEL(STAT_HP_REGEN, 1), NONE},
      {FROM(STAT_PARRY_PERCENT, 5, 1), NONE},
      {BASE_PLUS(STAT_EXTRA_TREASURE_PERCENT, 5, 1), NONE},
      {PER_LEVEL(STAT_SPIRIT_PER_KILL, 1), NONE}}},
    // Gimli: Strength of Beard, Orcslayer, Headsplitter, Gemfinder, Dwarven Resilience, Spirit of
    // the Forge
    {6,
     {{PER_LEVEL(STAT_MAX_HP, 10), NONE},
      {PER_LEVEL(STAT_DAMAGE_TO_ORCS, 2), NONE},
      {PER_LEVEL(STAT_CRIT_CHANCE_PERCENT, 2), NONE},
      {PER_LEVEL(STAT_GEMFINDER, 1), NONE},
      {PER_LEVEL(STAT_PARRY_PERCENT, 2), NONE},
      {PER_LEVEL(STAT_SPIRIT_REGEN, 1), NONE}}},
};

// The save slot the save menu has selected, else the one the player loaded.
static u32 currentSlot(void)
{
    u32 slot = g_SceneCurrent.id == SCENE_ID_SAVE_LOAD_MENU ? SCENE_ARG_SAVE_LOAD_SELECTED_SLOT
                                                             : g_LastSaveSlot;

    return g_SaveSlotPage * SAVE_SLOTS_PER_PAGE + slot;
}

static u8 *recordAddress(u32 slotIndex)
{
    return SRAM_BASE + LEGACY_SKILL_SAVE_OFF + slotIndex * LEGACY_SKILL_SAVE_RECORD_SIZE;
}

// The legacy skill record of the slot the player loaded, FALSE when it has none.
static bool32 readRecord(u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE])
{
    modSave_copy(record, recordAddress(currentSlot()), LEGACY_SKILL_SAVE_RECORD_SIZE);
    return record[0] == LEGACY_SKILL_RECORD_TAG;
}

#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT
// The legacy skill record of the character player 0 plays, record[0] != tag when it has none.
// Purchases change it; the slot gets it when the game is saved.
static u8 loadedRecord[LEGACY_SKILL_SAVE_RECORD_SIZE];
// The Legacy Skills option when that character was loaded. Off, its record is kept but its skills
// and their page are not there.
static bool shownThisGame;

// The Legacy Skills mod option's byte; any other value, the blank save's 0xFF included, is on.
#define LEGACY_SKILLS_OFF 1

static bool32 optionOn(void)
{
    u8 state;

    GLOBAL_MOD_OPTIONS_READ(legacySkillsState, &state);
    return state != LEGACY_SKILLS_OFF;
}
#endif

// The legacy skill record of the character player 0 plays, FALSE when it has none. The PC edition
// builds it with the skill rows, before the passives apply.
static bool32 playedRecord(u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE])
{
#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT
    memory_memcpy(record, loadedRecord, LEGACY_SKILL_SAVE_RECORD_SIZE);
    return shownThisGame && record[0] == LEGACY_SKILL_RECORD_TAG;
#else
    return readRecord(record);
#endif
}

static s32 passiveValue(const LegacyPassiveStat *stat, s32 level)
{
    return stat->base + stat->perLevel * level / stat->divisor;
}

void legacySkills_applyPassives(u8 playerIndex)
{
    u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE];
    const LegacyHeroPassives *passives;

    // A co-op partner's record stays on the other console.
    if (playerIndex != 0 || g_PlayerGlobals.activePlayerCount > 1 || !playedRecord(record) ||
        record[1] != PLAYER(playerIndex).heroId || record[1] >= LEGACY_HERO_COUNT)
    {
        return;
    }
    passives = &HeroPassives[record[1]];
    for (u32 skill = 0; skill < passives->count; skill++)
    {
        s32 level = record[2 + skill];
        if (level == 0 || level > LEGACY_SKILL_LEVEL_MAX)
        {
            continue;
        }
        for (u32 i = 0; i < LEGACY_PASSIVE_STATS; i++)
        {
            const LegacyPassiveStat *stat = &passives->stats[skill][i];
            if (stat->stat != STAT_NONE)
            {
                skill_passive_addStat(playerIndex, stat->stat, passiveValue(stat, level));
            }
        }
    }
}

#if SKILL_EXPANSION_LEGACY_ACTIVE_COUNT

// Each hero's actives by hero id. NULL: none yet.
static const LegacyActive *const HeroActives[LEGACY_HERO_COUNT] = {
    [0] = LegacyFrodoActives,
    [1] = LegacyLegolasActives,
    [2] = LegacyAragornActives,
    [3] = LegacyGandalfActives,
    [4] = LegacyEowynActives,
    [5] = LegacyGimliActives,
};

static const LegacyActive NoActive;

// The loaded hero's legacy active `i`; its def is NULL when it has none yet.
static const LegacyActive *heroActive(u32 heroId, u32 i)
{
    return HeroActives[heroId] != NULL ? &HeroActives[heroId][i] : &NoActive;
}

// A ROTK skill icon: a BG asset header (raw 4bpp tiles, no palette) and 4x3 tiles.
#define SKILL_ICON_HEADER_SIZE 4
#define SKILL_ICON_WIDTH 32
#define SKILL_ICON_HEIGHT 24
#define SKILL_ICON_TILE_BYTES (SKILL_ICON_WIDTH * SKILL_ICON_HEIGHT / 2)
#define SKILL_ICON_RAW_TILES 0x10
#define SKILL_ICON_FRAME_COLOR_LAST 7

typedef struct LegacySkillIcon
{
    u8 header[SKILL_ICON_HEADER_SIZE];
    u8 tiles[SKILL_ICON_TILE_BYTES];
} LegacySkillIcon;

// The loaded hero's Two Towers skill icons, valid while iconsBuilt, and their colors.
static LegacySkillIcon icons[LEGACY_SKILL_COUNT];
static bool iconsBuilt;
static SkillExpansionIconColors iconColors;

static u8 *iconPixelByte(u8 *tiles, u32 x, u32 y)
{
    return &tiles[((y / 8) * (SKILL_ICON_WIDTH / 8) + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2];
}

/**
 * Draw the hero's Two Towers skill icons in ROTK's style: the outer border of a stock icon (`frame`,
 * raw tiles) around the Two Towers art the PC host converted, which keeps its own colors.
 */
static void buildIcons(u32 heroId, const u8 *frame)
{
    const TwoTowersData *data = hackLib_twoTowersData();

    iconsBuilt = FALSE;
    if (data == NULL || data->skillArt == NULL || frame == NULL || frame[0] != SKILL_ICON_RAW_TILES)
    {
        return;
    }
    for (u32 skill = 0; skill < LEGACY_SKILL_COUNT; skill++)
    {
        const u8 *art = data->skillArt + (heroId * LEGACY_SKILL_COUNT + skill) * SKILL_ICON_TILE_BYTES;
        LegacySkillIcon *icon = &icons[skill];

        memory_memcpy(icon->header, frame, SKILL_ICON_HEADER_SIZE);
        memory_memcpy(icon->tiles, art, SKILL_ICON_TILE_BYTES);
        for (u32 y = 0; y < SKILL_ICON_HEIGHT; y++)
        {
            for (u32 x = 0; x < SKILL_ICON_WIDTH; x++)
            {
                u32 shift = (x & 1) * 4;
                u8 border = (*iconPixelByte((u8 *)frame + SKILL_ICON_HEADER_SIZE, x, y) >> shift) & 0xf;
                u8 *at = iconPixelByte(icon->tiles, x, y);

                if ((x <= 1 || x >= SKILL_ICON_WIDTH - 2 || y == 0 || y == SKILL_ICON_HEIGHT - 1) &&
                    border != 0 && border <= SKILL_ICON_FRAME_COLOR_LAST)
                {
                    *at = (*at & ~(0xf << shift)) | (border << shift);
                }
            }
        }
    }
    iconColors.colors = data->skillColors != NULL ? data->skillColors + heroId * SKILL_EXPANSION_ICON_OWN_COLOR_COUNT
                                                  : NULL;
    iconsBuilt = TRUE;
}

// Set legacy active `i`'s purchased and current level and its Spirit cost, which scales with the
// Two Towers level. The stock curves have five levels, so the row's levels stop there.
static void setRowLevel(u8 playerIndex, u32 i, u8 level)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    const LegacyActive *active = heroActive(loadedRecord[1], i);
    u32 row = SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + i;
    u8 rowLevel = level > LEGACY_SKILL_LEVEL_MAX ? 0 : level > ACTIVE_SKILL_MAX_LEVEL ? ACTIVE_SKILL_MAX_LEVEL : level;

    sx->activeSkills[row].spiritCost =
        active->spiritCost + active->spiritPerLevel * (level > LEGACY_SKILL_LEVEL_MAX ? 0 : level);
    *skillExpansion_activePurchased(playerIndex, row) = rowLevel;
    *skillExpansion_activeLevel(playerIndex, row) = rowLevel;
}

void legacySkills_buildRows(u8 heroId, u8 playerIndex)
{
    SkillExpansionPlayer *sx = &skillExpansion_players[playerIndex];
    u32 first;

    for (u32 i = 0; i < SKILL_EXPANSION_LEGACY_ACTIVE_COUNT; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + i;

        memory_memset(&sx->activeSkills[row], 0, sizeof(ActiveSkill));
        sx->activeSkillIcons[row] = NULL;
        sx->activeDefs[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW] = NULL;
        *skillExpansion_activePurchased(playerIndex, row) = 0;
    }
    // A co-op partner's record stays on the other console.
    if (playerIndex != 0)
    {
        return;
    }
    shownThisGame = FALSE;
    if (heroId >= LEGACY_HERO_COUNT)
    {
        loadedRecord[0] = LEGACY_SKILL_RECORD_EMPTY;
        return;
    }
    // Every Two Towers hero has the page; one not imported from The Two Towers starts with no ranks.
    if (!readRecord(loadedRecord) || loadedRecord[1] != heroId)
    {
        memory_memset(loadedRecord, 0, sizeof(loadedRecord));
        loadedRecord[0] = LEGACY_SKILL_RECORD_TAG;
        loadedRecord[1] = heroId;
    }
    // Off, the record still goes back to the slot on save.
    shownThisGame = optionOn();
    if (!shownThisGame)
    {
        return;
    }
    buildIcons(heroId, sx->activeSkillIcons[0]);
    first = 2 + HeroPassives[heroId].count;
    for (u32 i = 0; i < SKILL_EXPANSION_LEGACY_ACTIVE_COUNT && first + i < 2 + LEGACY_SKILL_COUNT;
         i++)
    {
        const LegacyActive *active = heroActive(heroId, i);
        const SkillExpansionActiveDef *def = active->def;
        u32 row = SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW + i;
        u8 level = loadedRecord[first + i];

        if (def == NULL)
        {
            continue;
        }
        memory_memcpy(&sx->activeSkills[row], &sx->activeSkills[def->baseRow], sizeof(ActiveSkill));
        sx->activeSkills[row].callback =
            active->cast != NULL ? active->cast : skillExpansion_castStockCallback;
        sx->activeSkillIcons[row] = iconsBuilt ? (const void *)&icons[first - 2 + i]
                                               : sx->activeSkillIcons[def->baseRow];
        sx->activeDefs[row - SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW] = def;
        *skillExpansion_activeCycleEnabled(playerIndex, row) = TRUE;
        setRowLevel(playerIndex, i, level);
    }
}

void legacySkills_slotCommitted(bool32 saved)
{
    u8 empty[LEGACY_SKILL_SAVE_RECORD_SIZE];
    const u8 *record = loadedRecord;

    if (!saved || loadedRecord[0] != LEGACY_SKILL_RECORD_TAG ||
        loadedRecord[1] != PLAYER(0).heroId)
    {
        memory_memset(empty, LEGACY_SKILL_RECORD_EMPTY, sizeof(empty));
        record = empty;
    }
    modSave_copy(recordAddress(currentSlot()), record, LEGACY_SKILL_SAVE_RECORD_SIZE);
}

// The text of each hero's first skill in The Two Towers; its 11 skills follow in record order.
static const u16 HeroSkillTextIds[LEGACY_HERO_COUNT] = {348, 359, 370, 381, 403, 392};

bool32 legacySkills_loaded(u8 playerIndex)
{
    return playerIndex == 0 && shownThisGame && loadedRecord[0] == LEGACY_SKILL_RECORD_TAG &&
           loadedRecord[1] == PLAYER(0).heroId && loadedRecord[1] < LEGACY_HERO_COUNT;
}

void legacySkills_setTestLevels(u8 level)
{
    if (!legacySkills_loaded(0))
    {
        return;
    }
    for (u32 skill = 0; skill < LEGACY_SKILL_COUNT; skill++)
    {
        loadedRecord[2 + skill] = level;
    }
    for (u32 i = 0; i < SKILL_EXPANSION_LEGACY_ACTIVE_COUNT &&
                    HeroPassives[loadedRecord[1]].count + i < LEGACY_SKILL_COUNT;
         i++)
    {
        if (heroActive(loadedRecord[1], i)->def != NULL)
        {
            setRowLevel(0, i, level);
        }
    }
}

u32 legacySkills_passiveCount(void)
{
    return legacySkills_loaded(0) ? HeroPassives[loadedRecord[1]].count : 0;
}

u32 legacySkills_activeCount(void)
{
    return legacySkills_loaded(0) ? LEGACY_SKILL_COUNT - HeroPassives[loadedRecord[1]].count : 0;
}

u8 legacySkills_level(u32 skill)
{
    u8 level = skill < LEGACY_SKILL_COUNT ? loadedRecord[2 + skill] : 0;

    return level > LEGACY_SKILL_LEVEL_MAX ? 0 : level;
}

// The Two Towers skill tree's rules by hero id, in the skill record's order: the player level a
// skill's first rank needs and the skill that needs a rank first (Two Towers 0x08057780, checked at
// 0x08039720). Further ranks cost only the skill point, up to LEGACY_SKILL_LEVEL_MAX.
typedef struct LegacySkillRule
{
    u8 requiredLevel;
    u8 prerequisite;
} LegacySkillRule;

#define NO_PREREQ 0xff

static const LegacySkillRule SkillRules[LEGACY_HERO_COUNT][LEGACY_SKILL_COUNT] = {
    // Frodo
    {{3, NO_PREREQ}, {8, 0}, {2, NO_PREREQ}, {5, 2}, {12, 2}, {1, NO_PREREQ}, {5, 5}, {8, 5}, {1, NO_PREREQ}, {3, 8}, {5, 8}},
    // Legolas
    {{1, NO_PREREQ}, {5, 0}, {8, 0}, {2, NO_PREREQ}, {3, 3}, {2, NO_PREREQ}, {5, 5}, {12, 5}, {2, NO_PREREQ}, {3, 8}, {12, 9}},
    // Aragorn
    {{2, NO_PREREQ}, {10, 0}, {2, NO_PREREQ}, {5, 2}, {8, 2}, {1, NO_PREREQ}, {5, 5}, {10, 6}, {10, 6}, {2, NO_PREREQ}, {5, 9}},
    // Gandalf
    {{2, NO_PREREQ}, {5, 0}, {2, NO_PREREQ}, {12, 2}, {1, NO_PREREQ}, {3, 4}, {2, NO_PREREQ}, {5, 6}, {5, 6}, {10, 7}, {10, 8}},
    // Eowyn
    {{2, NO_PREREQ}, {10, 0}, {5, NO_PREREQ}, {5, 2}, {10, 2}, {1, NO_PREREQ}, {5, 5}, {8, 6}, {10, 6}, {2, NO_PREREQ}, {5, 9}},
    // Gimli
    {{2, NO_PREREQ}, {3, 0}, {5, 0}, {2, NO_PREREQ}, {7, 2}, {10, 2}, {2, NO_PREREQ}, {6, 6}, {10, 6}, {1, NO_PREREQ}, {5, 9}},
};

bool32 legacySkills_buy(u32 skill)
{
    const LegacyHeroPassives *passives;
    const LegacySkillRule *rule;
    u8 level;

    // Legacy passives apply in single player only (legacySkills_applyPassives).
    if (!legacySkills_loaded(0) || g_PlayerGlobals.activePlayerCount > 1 ||
        skill >= LEGACY_SKILL_COUNT || PLAYER(0).skillPoints == 0)
    {
        return FALSE;
    }
    level = legacySkills_level(skill);
    rule = &SkillRules[loadedRecord[1]][skill];
    if (level >= LEGACY_SKILL_LEVEL_MAX ||
        (level == 0 && (rule->requiredLevel > PLAYER(0).level + 1 ||
                        (rule->prerequisite != NO_PREREQ && legacySkills_level(rule->prerequisite) == 0))))
    {
        return FALSE;
    }
    loadedRecord[2 + skill] = level + 1;
    scene_skillTree_spendSkillPoint(0);
    passives = &HeroPassives[loadedRecord[1]];
    if (skill < passives->count)
    {
        for (u32 i = 0; i < LEGACY_PASSIVE_STATS; i++)
        {
            const LegacyPassiveStat *stat = &passives->stats[skill][i];

            if (stat->stat == STAT_NONE)
            {
                continue;
            }
            if (level != 0)
            {
                skill_passive_subtractStat(0, stat->stat, passiveValue(stat, level));
            }
            skill_passive_addStat(0, stat->stat, passiveValue(stat, level + 1));
        }
    }
    else if (skill - passives->count < SKILL_EXPANSION_LEGACY_ACTIVE_COUNT &&
             heroActive(loadedRecord[1], skill - passives->count)->def != NULL)
    {
        setRowLevel(0, skill - passives->count, level + 1);
    }
    return TRUE;
}

const SkillExpansionIconColors *legacySkills_iconColors(const void *icon)
{
    if (!iconsBuilt || iconColors.colors == NULL || (const u8 *)icon < (const u8 *)icons ||
        (const u8 *)icon >= (const u8 *)(icons + LEGACY_SKILL_COUNT))
    {
        return NULL;
    }
    return &iconColors;
}

const void *legacySkills_icon(u32 skill)
{
    return legacySkills_loaded(0) && iconsBuilt && skill < LEGACY_SKILL_COUNT ? &icons[skill] : NULL;
}

const char *legacySkills_text(u32 skill)
{
    const TwoTowersData *data = hackLib_twoTowersData();
    u32 language = text_getLanguage();
    u32 id;

    if (data == NULL || !legacySkills_loaded(0) || skill >= LEGACY_SKILL_COUNT ||
        language >= LANGUAGE_COUNT)
    {
        return NULL;
    }
    id = HeroSkillTextIds[loadedRecord[1]] + skill;
    return id < data->stringCount ? data->strings[language][id] : NULL;
}

#if HACK_mod_options
static const char *const OffOnNames[2] = {"Off", "On"};

static u8 get_enabled(void)
{
    return optionOn();
}

static void set_enabled(u8 value)
{
    u8 state = value ? 0 : LEGACY_SKILLS_OFF;

    GLOBAL_MOD_OPTIONS_WRITE(legacySkillsState, &state);
}

const struct ModOptionEntry LegacySkillsOptions[LEGACY_SKILLS_OPTION_COUNT] = {
    {"Legacy Skills", OffOnNames, 2, 0, get_enabled, set_enabled,
     "The Two Towers skill page.\nApplies when a game loads."},
};
#endif

#endif
