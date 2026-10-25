#include "patches/gauntlet/gauntlet_level.h"

#include "font.h"
#include "patches/gauntlet/gauntlet.h"
#include "patches/gauntlet/gauntlet_boon_effects.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "player.h"
#include "rng.h"
#include "skill.h"
#include "text.h"
#include "variables.h"

#define STAT_OFFER_POINTS 3
#define BASE_STAT_COUNT 5
#define NAME_MAX 18

static const char *const BaseStatNames[BASE_STAT_COUNT] = {"Courage", "Defense", "Health",
                                                           "Accuracy", "Strength"};
static const char *const BaseStatDescriptions[BASE_STAT_COUNT] = {
    "More Spirit for skills and stronger skill effects.",
    "Better armor: enemy hits deal less damage.",
    "More maximum HP.",
    "Your attacks land more often and crit more.",
    "Your weapon attacks deal more damage.",
};

/** The base stat field for index 0..4, in HeroBaseStats order after maxHp. */
static u16 *baseStat(u8 playerIndex, u32 index)
{
    return &(&PLAYER(playerIndex).baseStats.courage)[index];
}

void gauntletLevel_refreshStats(u8 playerIndex)
{
    // player_rebuildStats keeps the old baseStats.maxHp, so Health gains need it recomputed.
    PLAYER(playerIndex).baseStats.maxHp =
        player_computeMaxHp(playerIndex, PLAYER(playerIndex).baseStats);
    player_rebuildStats(playerIndex);
    gauntletBoonEffects_statsRebuilt();
}

void gauntletLevel_settle(u8 playerIndex)
{
    u32 next = PLAYER(playerIndex).level;

    while (PLAYER(playerIndex).attributePoints != 0)
    {
        (*baseStat(playerIndex, next++ % BASE_STAT_COUNT))++;
        PLAYER(playerIndex).attributePoints--;
    }
    PLAYER(playerIndex).skillPoints = 0;
    gauntletLevel_refreshStats(playerIndex);
}

static bool32 rowExists(u8 playerIndex, u32 row)
{
    return row != ACTIVE_SKILL_NONE && PLAYER_ACTIVE_SKILL(playerIndex, row).callback != NULL;
}

static bool32 rowOffered(u8 playerIndex, u32 row)
{
    return rowExists(playerIndex, row) &&
           *skillExpansion_activeLevel(playerIndex, row) < ACTIVE_SKILL_MAX_LEVEL;
}

static bool32 taken(const u8 offers[3], u32 filled, u8 offer)
{
    u32 i;

    for (i = 0; i < filled; i++)
    {
        if (offers[i] == offer)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void gauntletLevel_roll(u8 playerIndex, u8 offers[3])
{
    u32 filled = 0;
    u32 attempt;

    // Up to two skill choices, then fill with stats.
    for (attempt = 0; attempt < 48 && filled < 2; attempt++)
    {
        u8 row = gauntlet_randBelow(SKILL_EXPANSION_ACTIVE_ROW_COUNT);

        if (rowOffered(playerIndex, row) && !taken(offers, filled, row))
        {
            offers[filled++] = row;
        }
    }
    for (attempt = 0; attempt < 16 && filled < 3; attempt++)
    {
        u8 stat = LEVEL_OFFER_STAT + gauntlet_randBelow(BASE_STAT_COUNT);

        if (!taken(offers, filled, stat))
        {
            offers[filled++] = stat;
        }
    }
    while (filled < 3)
    {
        offers[filled++] = LEVEL_OFFER_NONE;
    }
}

void gauntletLevel_apply(u8 playerIndex, u8 offer)
{
    if (offer >= LEVEL_OFFER_STAT && offer < LEVEL_OFFER_STAT + BASE_STAT_COUNT)
    {
        *baseStat(playerIndex, offer - LEVEL_OFFER_STAT) += STAT_OFFER_POINTS;
        gauntletLevel_refreshStats(playerIndex);
        return;
    }
    if (offer < SKILL_EXPANSION_ACTIVE_ROW_COUNT && rowOffered(playerIndex, offer))
    {
        u8 *level = skillExpansion_activeLevel(playerIndex, offer);

        (*level)++;
        *skillExpansion_activePurchased(playerIndex, offer) = *level;
    }
}

/** The row's skill tree description, which starts with the skill's name. */
static const char *rowDescription(u8 playerIndex, u32 row)
{
    u32 heroId = PLAYER(playerIndex).heroId;
    const SkillExpansionHeroKit *kit = skillExpansion_getHeroKit(heroId);
    u32 artHeroId = kit != NULL ? kit->artHeroId : heroId;
    const char *description = NULL;
    u32 textRow = row;

    if (row >= SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW)
    {
        const SkillExpansionActiveDef *def = skillExpansion_activeDef(playerIndex, row);

        if (def == NULL)
        {
            return "";
        }
        description = def->description;
        textRow = def->baseRow;
    }
    else if (kit != NULL)
    {
        description = kit->descriptions[row];
    }
    else
    {
        description = skillExpansion_getStockActiveDescription(heroId, row);
    }
    if (description == NULL && artHeroId < ARRAY_COUNT(ActiveSkillDescriptionBaseTextIds))
    {
        description = text_getString(ActiveSkillDescriptionBaseTextIds[artHeroId] + textRow);
    }
    return description != NULL ? description : "";
}

/** Copy the name at the start of a description, up to " (" or " -". */
static char *copyName(char *out, const char *description)
{
    u32 n;

    for (n = 0; n < NAME_MAX && description[n] != '\0'; n++)
    {
        if (description[n] == ' ' && (description[n + 1] == '(' || description[n + 1] == '-'))
        {
            break;
        }
        *out++ = description[n];
    }
    *out = '\0';
    return out;
}

static char *appendText(char *out, const char *text)
{
    while (*text != '\0')
    {
        *out++ = *text++;
    }
    *out = '\0';
    return out;
}

void gauntletLevel_label(u8 playerIndex, u8 offer, char *out)
{
    if (offer >= LEVEL_OFFER_STAT && offer < LEVEL_OFFER_STAT + BASE_STAT_COUNT)
    {
        out = appendText(out, "+3 ");
        appendText(out, BaseStatNames[offer - LEVEL_OFFER_STAT]);
        return;
    }
    if (offer >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        *out = '\0';
        return;
    }
    if (*skillExpansion_activeLevel(playerIndex, offer) == 0)
    {
        out = appendText(out, "Learn ");
        copyName(out, rowDescription(playerIndex, offer));
        return;
    }
    out = copyName(out, rowDescription(playerIndex, offer));
    out = appendText(out, " Lv");
    *out++ = '1' + *skillExpansion_activeLevel(playerIndex, offer);
    *out = '\0';
}

const char *gauntletLevel_description(u8 playerIndex, u8 offer)
{
    const char *description;
    u32 n;

    if (offer >= LEVEL_OFFER_STAT && offer < LEVEL_OFFER_STAT + BASE_STAT_COUNT)
    {
        return BaseStatDescriptions[offer - LEVEL_OFFER_STAT];
    }
    if (offer >= SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        return "";
    }
    // The skill text past its name and "(Level @1) - " prefix, at the level this offer gives.
    // '@2' is the Spirit cost.
    font_setIntTemplateArg0(*skillExpansion_activeLevel(playerIndex, offer) + 1);
    font_setIntTemplateArg(skill_active_getSpiritCost(playerIndex, offer), 1);
    description = rowDescription(playerIndex, offer);
    for (n = 0; description[n] != '\0'; n++)
    {
        if (description[n] == '-' && description[n + 1] == ' ')
        {
            return description + n + 2;
        }
    }
    return description;
}

static char *appendNumber(char *out, u32 value)
{
    char digits[10];
    u32 count = 0;
    do
    {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value != 0);
    while (count != 0)
    {
        *out++ = digits[--count];
    }
    *out = '\0';
    return out;
}

void gauntletLevel_progress(u8 playerIndex, u8 offer, char *out)
{
    u32 current;
    u32 gain;
    if (offer >= LEVEL_OFFER_STAT && offer < LEVEL_OFFER_STAT + BASE_STAT_COUNT)
    {
        out = appendText(out, BaseStatNames[offer - LEVEL_OFFER_STAT]);
        current = *baseStat(playerIndex, offer - LEVEL_OFFER_STAT);
        gain = STAT_OFFER_POINTS;
    }
    else if (offer == ACTIVE_SKILL_HERBAL_HEALING &&
             skillExpansion_getHeroKit(PLAYER(playerIndex).heroId) == NULL)
    {
        current = *skillExpansion_activeLevel(playerIndex, offer);
        out = appendText(out, "Extra heal: +");
        out = appendNumber(out, current > 1 ? (current - 1) * 10 : 0);
        out = appendText(out, "% -> +");
        out = appendNumber(out, current * 10);
        appendText(out, "% max HP");
        return;
    }
    else if (offer < SKILL_EXPANSION_ACTIVE_ROW_COUNT)
    {
        out = appendText(out, "Skill rank");
        current = *skillExpansion_activeLevel(playerIndex, offer);
        gain = 1;
    }
    else
    {
        *out = '\0';
        return;
    }
    out = appendText(out, ": ");
    out = appendNumber(out, current);
    out = appendText(out, " -> ");
    appendNumber(out, current + gain);
}
