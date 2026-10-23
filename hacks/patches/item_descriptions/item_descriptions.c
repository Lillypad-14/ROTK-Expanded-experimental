#include "font.h"
#include "item.h"
#include "memory.h"
#include "player.h"
#include "stats.h"
#include "text.h"
#include "variables.h"
#if HACK_ringwraith && HACK_skill_expansion
#include "patches/ringwraith/wraith_gear.h"
#endif

#define ITEM_DESCRIPTIONS_UNIQUE_COUNT 26
#define ITEM_DESCRIPTIONS_RUNE_DESC_BASE (TEXT_ID_RUNE_NOT_AVAILABLE + 1)
#define ITEM_DESCRIPTIONS_PANEL_BUFFER_SIZE 200

static const char *const backpackInstructions[LANGUAGE_COUNT] = {
    "Keep in backpack.",  "Garder dans le sac.",   "Im Rucksack aufbewahren.",
    "Tieni nello zaino.", "Llevar en la mochila.",
};

static const char *const uniqueTags[LANGUAGE_COUNT] = {
    "Unique.", "Unique.", "Einzigartig.", "Unico.", "\227nico.",
};

static const char *const corruptItemTags[LANGUAGE_COUNT] = {
    "Corrupt.", "Corrompt.", "Verdorben.", "Corruzione.", "Corromp.",
};

static const char *const corruptRuneTags[LANGUAGE_COUNT] = {
    "Corrupt.", "Corrompt.", "Verderbt.", "Corruzione.", "Corrompe.",
};

static const char *const classPrefixes[LANGUAGE_COUNT] = {
    "", "Uniquement ", "Nur ", "Solo ", "Solo ",
};

static const char *const classSuffixes[LANGUAGE_COUNT] = {
    " only.", ".", ".", ".", ".",
};

static const char *const classConjunctions[LANGUAGE_COUNT] = {
    " and ", " et ", " und ", " e ", " y ",
};

static char *appendLiteral(char *dst, const char *src)
{
    char *limit = g_TextStringBuffer + TEXT_STRING_BUFFER_SIZE - 1;

    while (*src != '\0' && dst < limit)
    {
        *dst++ = *src++;
    }
    *dst = '\0';
    return dst;
}

static char *appendTag(char *dst, const char *tag)
{
    if (dst != g_TextStringBuffer)
    {
        dst = appendLiteral(dst, dst[-1] == '.' ? " " : ". ");
    }
    return appendLiteral(dst, tag);
}

static char *appendText(char *dst, TextId id)
{
    s32 remaining = g_TextStringBuffer + TEXT_STRING_BUFFER_SIZE - dst;

    if (text_decodeHuffmanString(id, (u8 *)dst, remaining) != TEXT_DECODE_OK)
    {
        *dst = '\0';
        return dst;
    }
    while (*dst != '\0')
    {
        dst++;
    }
    return dst;
}

static void appendCarriedUsage(char *dst, Item item)
{
    u32 flags = ITEM_BASE_METADATA(item).flags.p;
    u32 classes = flags & ITEM_FLAG_CLASS_ALL;
    u8 language = text_getLanguage();
    u8 hero;
    u8 remaining = 0;
    bool first = TRUE;

    if ((flags & ITEM_FLAG_CARRIED_PASSIVE) == 0)
    {
        return;
    }

    dst = appendLiteral(dst, "\n");
    dst = appendLiteral(dst, backpackInstructions[language]);
    if (classes == ITEM_FLAG_CLASS_ALL)
    {
        return;
    }

    for (hero = HERO_ID_FRODO; hero <= HERO_ID_SAM; hero++)
    {
        u8 classHero = hero == HERO_ID_SAM ? HERO_ID_FRODO : hero;

        if (classes & ITEM_FLAG_CLASS_BIT(classHero))
        {
            remaining++;
        }
    }
    dst = appendLiteral(dst, "\n");
    dst = appendLiteral(dst, classPrefixes[language]);
    for (hero = HERO_ID_FRODO; hero <= HERO_ID_SAM; hero++)
    {
        u8 classHero = hero == HERO_ID_SAM ? HERO_ID_FRODO : hero;

        if ((classes & ITEM_FLAG_CLASS_BIT(classHero)) == 0)
        {
            continue;
        }
        if (!first)
        {
            dst = appendLiteral(dst, remaining == 1 ? classConjunctions[language] : ", ");
        }
        dst = appendText(dst, TEXT_ID_HERO_NAME_BASE + hero);
        remaining--;
        first = FALSE;
    }
    appendLiteral(dst, classSuffixes[language]);
}

static char *describeUnique(Item item)
{
    char *dst = g_TextStringBuffer;
    char *end;

    // Carried passives apply their base records, not carved runes.
    if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_CARRIED_PASSIVE)
    {
        item.d.runeIndex = RUNE_NONE;
    }
    memory_memset(dst, 0, TEXT_STRING_BUFFER_SIZE);
    end = item_affix_accumulateStats(dst, item);
    if (item.d.baseIndex <= UNIQUE_ID_NIGHTFELL_BOW)
    {
        end = appendTag(end, uniqueTags[text_getLanguage()]);
    }
    if (item.d.baseIndex == UNIQUE_ID_FORGOTTEN_PALANTIR ||
        item.d.baseIndex == UNIQUE_ID_CRACKED_PALANTIR)
    {
        end = appendTag(end, corruptItemTags[text_getLanguage()]);
    }
    appendCarriedUsage(end, item);
    return dst;
}

static char *formatRuneStat(char *dst, u8 statIndex, s16 value)
{
    bool property = statIndex >= STAT_EXTRA_PROJECTILES && statIndex <= STAT_MEDITATIVE &&
                    statIndex != STAT_WISDOM;
    bool percent =
        statIndex == STAT_SPEED_PERCENT || statIndex == STAT_HP_PERCENT_PER_KILL ||
        (statIndex >= STAT_EXTRA_GEMS_PERCENT && statIndex <= STAT_CRIT_CHANCE_PERCENT) ||
        statIndex == STAT_PUSH_FOE_BACK ||
        (statIndex >= STAT_CORRUPTION_RESIST_PERCENT && statIndex <= STAT_BLOCK_PERCENT);

    if (statIndex <= STAT_DAMAGE_IMPALE)
    {
        text_formatInt(value / 2, dst);
        while (*dst != '\0')
        {
            dst++;
        }
        dst = appendLiteral(dst, "-");
    }
    else if (!property && value > 0)
    {
        dst = appendLiteral(dst, "+");
    }
    if (!property)
    {
        text_formatInt(value, dst);
        while (*dst != '\0')
        {
            dst++;
        }
        if (!percent)
        {
            dst = appendLiteral(dst, " ");
        }
    }
    if (statIndex == STAT_MEDITATIVE)
    {
        return appendText(dst, TEXT_ID_STAND_STILL_TO_HEAL_ONESELF);
    }
    return appendText(dst, TEXT_ID_STAT_MELEE_DAMAGE + statIndex);
}

static char *describeRune(u8 runeIndex)
{
    const RuneStats *rune = &Runes[runeIndex];
    s16 values[STAT_NONE] = {0};
    char *dst = g_TextStringBuffer;
    u8 statIndex;
    bool first = TRUE;

    if (rune->stat0 != STAT_NONE)
    {
        values[rune->stat0] += rune->val0;
    }
    if (rune->stat1 != STAT_NONE)
    {
        values[rune->stat1] += rune->val1;
    }
    *dst = '\0';
    for (statIndex = 0; statIndex <= STAT_HP_FROM_HERBS; statIndex++)
    {
        if (values[statIndex] == 0)
        {
            continue;
        }
        if (!first)
        {
            dst = appendLiteral(dst, ", ");
        }
        dst = formatRuneStat(dst, statIndex, values[statIndex]);
        first = FALSE;
    }
    if (runeIndex >= RUNE_MORGUL_1_ORC_DOOM_KING && runeIndex <= RUNE_MORGUL_3_BLACKNESS)
    {
        appendTag(dst, corruptRuneTags[text_getLanguage()]);
    }
    return g_TextStringBuffer;
}

char *itemDescriptions_getString(s32 id)
{
    if (id >= ITEM_DESCRIPTIONS_RUNE_DESC_BASE &&
        id < ITEM_DESCRIPTIONS_RUNE_DESC_BASE + RUNE_COUNT)
    {
        return describeRune(id - ITEM_DESCRIPTIONS_RUNE_DESC_BASE);
    }
    if (text_decodeHuffmanString(id, (u8 *)g_TextStringBuffer, TEXT_STRING_BUFFER_SIZE) ==
        TEXT_DECODE_OK)
    {
        return g_TextStringBuffer;
    }
    return NULL;
}

char *itemDescriptions_drawInfo(char *buffer, u32 playerIndex, Item item, const u16 *x, u16 *y,
                                u16 *tileIndex)
{
    char *line = item_affix_formatName(buffer, item, 220);
    u8 minimumLevel;

    font_setStyle(FONT_STYLE_8PX_A, g_PlayerHuds[playerIndex].textColor, 0);
    *tileIndex = font_drawLinesInBox(*tileIndex, *x, *y, 118, 16, &line, 0);

    memory_memset(buffer, 0, ITEM_DESCRIPTIONS_PANEL_BUFFER_SIZE);
    minimumLevel = item_getMinLevel(item.word);
    font_setStyle(FONT_STYLE_8PX_A, PLAYER(playerIndex).level + 1 < minimumLevel ? 6 : 9, 0);
    *y = 96;
    font_setTemplateArg0(text_formatInt(minimumLevel, buffer));
    line = text_getString(TEXT_ID_MIN_LEVEL_1);
    *tileIndex = font_drawLines(*tileIndex, *x, *y, 118, line, 0);

    font_setStyle(FONT_STYLE_8PX_A, 3, 0);
#if HACK_ringwraith && HACK_skill_expansion
    if (wraithGear_isWraithItem(item))
    {
        return wraithGear_describe(item);
    }
#endif
    if (ITEM_TYPE(item) == ITEM_TYPE_UNIQUE && item.d.baseIndex < ITEM_DESCRIPTIONS_UNIQUE_COUNT)
    {
        return describeUnique(item);
    }
    memory_memset(buffer, 0, ITEM_DESCRIPTIONS_PANEL_BUFFER_SIZE);
    item_affix_accumulateStats(buffer, item);
    return buffer;
}
