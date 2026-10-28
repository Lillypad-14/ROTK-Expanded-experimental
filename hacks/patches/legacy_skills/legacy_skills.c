#if !HACK_skill_expansion || !HACK_mod_save
#error "legacy_skills requires skill_expansion and mod_save"
#endif

#include "patches/legacy_skills/legacy_skills.h"
#include "patches/mod_save/save_layout.h"
#include "patches/mod_save/save_slots.h"
#include "patches/mod_save/save_sram.h"
#include "player.h"
#include "scene.h"
#include "scene/saveLoadMenu.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

#define LEGACY_SKILL_RECORD_TAG 0x01
#define LEGACY_SKILL_COUNT 11
#define LEGACY_PASSIVE_STATS 2
#define LEGACY_HERO_COUNT 6
#define LEGACY_PASSIVE_MAX 6
#define LEGACY_SKILL_LEVEL_MAX 20

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

// The Two Towers passives by hero id. A skill with no ROTK mechanic uses the closest stat.
static const LegacyHeroPassives HeroPassives[LEGACY_HERO_COUNT] = {
    // Frodo: Resist One Ring, Strength of the Shire, Rummage, Sneak Attack, Duck
    {5,
     {{PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 4), NONE},
      {PER_LEVEL(STAT_HP_REGEN, 1), PER_LEVEL(STAT_SPIRIT_REGEN, 1)},
      {PER_LEVEL(STAT_EXTRA_TREASURE_PERCENT, 10), NONE},
      {PER_LEVEL(STAT_BACKSTAB, 2), NONE},
      {PER_LEVEL(STAT_MELEE_ARMOR, 1), PER_LEVEL(STAT_MISSILE_ARMOR, 1)}}},
    // Legolas: Meditation, Fleet of Foot, Orc Slayer, Hawk Eye, Critical Shot
    {5,
     {{PER_LEVEL(STAT_MEDITATIVE, 1), NONE},
      {PER_LEVEL(STAT_SPEED_PERCENT, 3), NONE},
      {PER_LEVEL(STAT_DAMAGE_TO_ORCS, 2), NONE},
      {PER_LEVEL(STAT_DAMAGE_WITH_BOW, 1), NONE},
      {FROM(STAT_CRIT_CHANCE_PERCENT, 10, 2), NONE}}},
    // Aragorn: Dual Wield, Numenorean Blood, Shield Offense, Parry, Ferocity
    {5,
     {{PER_LEVEL(STAT_ARAGORN_DUAL_WIELD_DAMAGE, 1), NONE},
      {PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 5), PER_LEVEL(STAT_DAMAGE_TO_NAZGUL, 2)},
      {{STAT_DAMAGE_WITH_SHIELD, 0, 1, 2}, PER_LEVEL(STAT_BLOCK_PERCENT, 1)},
      {FROM(STAT_PARRY_PERCENT, 5, 1), NONE},
      {FROM(STAT_HP_PERCENT_PER_KILL, 5, 1), NONE}}},
    // Gandalf: Elusion, Inspire, Wisdom, Flame of Anor
    {4,
     {{FROM(STAT_CORRUPTION_RESIST_PERCENT, 20, 4), NONE},
      {PER_LEVEL(STAT_SPIRIT_REGEN_PERCENT, 25), NONE},
      {PER_LEVEL(STAT_WISDOM, 1), NONE},
      {FROM(STAT_REVIVE_CHANCE_PERCENT, 20, 2), NONE}}},
    // Eowyn: Strength of Heart, Iron Will, Stand Fast, Eyes of Rohan, Ferocity
    {5,
     {{PER_LEVEL(STAT_CORRUPTION_RESIST_PERCENT, 5), PER_LEVEL(STAT_DAMAGE_TO_NAZGUL, 2)},
      {PER_LEVEL(STAT_HP_REGEN, 1), NONE},
      {FROM(STAT_PARRY_PERCENT, 5, 1), NONE},
      {PER_LEVEL(STAT_EXTRA_TREASURE_PERCENT, 10), NONE},
      {PER_LEVEL(STAT_SPIRIT_PER_KILL, 1), NONE}}},
    // Gimli: Strength of Beard, Orcslayer, Headsplitter, Gemfinder, Dwarven Resilience, Spirit of
    // the Forge
    {6,
     {{PER_LEVEL(STAT_MAX_HP, 10), NONE},
      {PER_LEVEL(STAT_DAMAGE_TO_ORCS, 2), NONE},
      {FROM(STAT_CRIT_CHANCE_PERCENT, 10, 3), NONE},
      {PER_LEVEL(STAT_GEMFINDER, 1), NONE},
      {FROM(STAT_DODGE_PERCENT, 10, 2), NONE},
      {PER_LEVEL(STAT_SPIRIT_REGEN, 1), NONE}}},
};

// The legacy skill record of the slot the player loaded, FALSE when it has none.
static bool32 readRecord(u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE])
{
    u32 slot = g_SceneCurrent.id == SCENE_ID_SAVE_LOAD_MENU ? SCENE_ARG_SAVE_LOAD_SELECTED_SLOT
                                                             : g_LastSaveSlot;
    u32 index = g_SaveSlotPage * SAVE_SLOTS_PER_PAGE + slot;

    if (slot >= SAVE_SLOTS_PER_PAGE)
    {
        return FALSE;
    }
    modSave_copy(record, SRAM_BASE + LEGACY_SKILL_SAVE_OFF + index * LEGACY_SKILL_SAVE_RECORD_SIZE,
                 LEGACY_SKILL_SAVE_RECORD_SIZE);
    return record[0] == LEGACY_SKILL_RECORD_TAG;
}

void legacySkills_applyPassives(u8 playerIndex)
{
    u8 record[LEGACY_SKILL_SAVE_RECORD_SIZE];
    const LegacyHeroPassives *passives;

    // A co-op partner's record stays on the other console.
    if (playerIndex != 0 || g_PlayerGlobals.activePlayerCount > 1 || !readRecord(record) ||
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
                skill_passive_addStat(playerIndex, stat->stat,
                                      stat->base + stat->perLevel * level / stat->divisor);
            }
        }
    }
}
