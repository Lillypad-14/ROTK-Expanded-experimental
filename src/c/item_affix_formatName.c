#include "font.h"
#include "game.h"
#include "item.h"
#include "player.h"
#include "save.h"
#include "scene.h"
#include "text.h"
#include "text_ids.h"
#include "variables.h"

// Select the gameplay or menu font-color bias for the given player.
static inline void setItemNameColor(u8 playerIndex, u8 gameplayColor, u8 menuColor)
{
    if (g_SceneCurrent.id == SCENE_ID_GAMEPLAY)
    {
        g_PlayerHuds[playerIndex].textColor = gameplayColor;
    }
    else
    {
        g_PlayerHuds[playerIndex].textColor = menuColor;
    }
}

// Replace trailing words with " ..." when the name exceeds the current font's width threshold.
static inline void truncateNameWithEllipsis(char *dst, char *nameStart, u16 maxWidth)
{
    if (font_getStringWidth(nameStart) > maxWidth)
    {
        do
        {
            *--dst = '\0';
        } while (font_getStringWidth(nameStart) > maxWidth);

        {
            char *end = dst;
            // Reserve four source bytes, then backtrack to a space or the name's start.
            // BUG: A fitted prefix shorter than four bytes backtracks before nameStart.
            while (dst > end - 4 || (*dst != ' ' && dst != nameStart))
            {
                *dst-- = '\0';
            }
        }
        // BUG: Reserving four bytes does not guarantee enough pixel width for " ...".
        *dst++ = ' ';
        *dst++ = '.';
        *dst++ = '.';
        *dst = '.';
    }
}

/**
 * Build an item's localized display name and set the active player's HUD text color.
 * Ordinary items include affixes, a two-handed marker, and an arrow count as applicable.
 * Unique items use only their base name, without a two-handed marker or width truncation.
 * The text language package and current font must be initialized.
 *
 * @param dst Zero-filled buffer large enough for the full untruncated name and terminator.
 * @param item Valid, nonempty item handle, not a gem-stack payload.
 * @param maxWidth Pixel truncation threshold in the current font, not a buffer-size limit
 *                 or a guaranteed final width. Overlong ordinary names end in " ...".
 *                 Very small thresholds can cause writes before dst.
 *
 * @return The original dst pointer.
 *
 * @romaddress 0x0802b064
 */
char *item_affix_formatName(char *dst, Item item, u16 maxWidth)
{
    char *nameStart;
    u8 playerIndex = ACTIVE_PLAYER_INDEX;

    nameStart = dst;

    if (ITEM_HAS_SUFFIX(item) ||
        (item.d.itemType == ITEM_TYPE_UNIQUE && item.d.baseIndex < UNIQUE_CARRIED_PASSIVE_BAND_END))
    {
        // HACK: Identical branches keep the color constant next to its store.
        if (g_SceneCurrent.id == SCENE_ID_GAMEPLAY)
        {
            g_PlayerHuds[playerIndex].textColor = HUD_TEXT_COLOR_GAMEPLAY_RED;
        }
        else
        {
            g_PlayerHuds[playerIndex].textColor = HUD_TEXT_COLOR_MENU_RED;
        }
        if (item.d.itemType == ITEM_TYPE_UNIQUE &&
            item.d.baseIndex < UNIQUE_CARRIED_PASSIVE_BAND_END)
        {
            text_appendString(dst, ITEM_BASE(item).nameId);
            {
                u32 baseIndex = item.d.baseIndex;
                if (baseIndex >= UNIQUE_WEAPON_BAND_END)
                {
                    return nameStart;
                }
            }
            setItemNameColor(playerIndex, HUD_TEXT_COLOR_GAMEPLAY_BLUE, HUD_TEXT_COLOR_MENU_BLUE);
            return nameStart;
        }
    }
    else
    {
        if ((ITEM_HAS_PREFIX(item) && item.d.itemType != ITEM_TYPE_ARROW) ||
            (item.d.itemType == ITEM_TYPE_UNIQUE && item.d.baseIndex < UNIQUE_ITEM_COUNT))
        {
            setItemNameColor(playerIndex, HUD_TEXT_COLOR_GAMEPLAY_GREEN, HUD_TEXT_COLOR_MENU_GREEN);

            if (item.d.itemType == ITEM_TYPE_UNIQUE && item.d.baseIndex < UNIQUE_ITEM_COUNT)
            {
                if (item.d.baseIndex < UNIQUE_WEAPON_BAND_END ||
                    item.d.baseIndex >= UNIQUE_CARRIED_PASSIVE_BAND_END)
                {
                    setItemNameColor(playerIndex, HUD_TEXT_COLOR_GAMEPLAY_BLUE,
                                     HUD_TEXT_COLOR_MENU_BLUE);
                }
                text_appendString(dst, ITEM_BASE(item).nameId);
                return nameStart;
            }
        }
        else
        {
            if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_ANCIENT)
            {
                setItemNameColor(playerIndex, HUD_TEXT_COLOR_GAMEPLAY_BLUE,
                                 HUD_TEXT_COLOR_MENU_BLUE);
            }
            else
            {
                setItemNameColor(playerIndex, HUD_TEXT_COLOR_GAMEPLAY_YELLOW,
                                 HUD_TEXT_COLOR_MENU_YELLOW);
            }
        }
    }

    if (g_SaveGlobals.d.language != LANGUAGE_EN)
    {
        dst = text_appendString(dst, ITEM_BASE(item).nameId);
        {
            u32 noAffix = ITEM_AFFIXES_NONE;
            // BUG: An arrow stack's count byte also triggers this affix separator.
            if (item.halfwords.affixes != noAffix)
            {
                // French typography: space before ':'
                if (g_SaveGlobals.d.language == LANGUAGE_FR)
                {
                    *dst++ = ' ';
                }
                if (!LANGUAGE_SUFFIX_FIRST(g_SaveGlobals.d.language))
                {
                    *dst++ = ':';
                }
                *dst++ = ' ';
            }
        }
        if (!LANGUAGE_SUFFIX_FIRST(g_SaveGlobals.d.language))
        {
            if (ITEM_HAS_PREFIX(item) && item.d.itemType != ITEM_TYPE_ARROW)
            {
                dst = text_appendString(dst, ITEM_PREFIX(item).nameId);
            }
            if (ITEM_HAS_SUFFIX(item))
            {
                if (ITEM_HAS_PREFIX(item))
                {
                    *dst++ = ',';
                    *dst++ = ' ';
                }
                dst = text_appendString(dst, ITEM_SUFFIX(item).nameId);
            }
        }
        else
        {
            if (ITEM_HAS_SUFFIX(item))
            {
                dst = text_appendString(dst, ITEM_SUFFIX(item).nameId);
                if (ITEM_HAS_PREFIX(item))
                {
                    *dst++ = ',';
                    *dst++ = ' ';
                }
            }
            if (ITEM_HAS_PREFIX(item) && item.d.itemType != ITEM_TYPE_ARROW)
            {
                dst = text_appendString(dst, ITEM_PREFIX(item).nameId);
            }
        }
    }
    else
    {
        if (ITEM_HAS_PREFIX(item) && item.d.itemType != ITEM_TYPE_ARROW)
        {
            dst = text_appendString(dst, ITEM_PREFIX(item).nameId);
            *dst++ = ' ';
        }
        dst = text_appendString(dst, ITEM_BASE(item).nameId);
        if (ITEM_HAS_SUFFIX(item))
        {
            *dst++ = ' ';
            dst = text_appendString(dst, ITEM_SUFFIX(item).nameId);
        }
    }

    if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_TWO_HANDED)
    {
        *dst++ = ' ';
        dst = text_appendString(dst, TEXT_ID_2H);
    }

    {
        if (item.d.itemType == ITEM_TYPE_ARROW)
        {
            // French typography: space before ':'
            if (g_SaveGlobals.d.language == LANGUAGE_FR)
            {
                *dst++ = ' ';
            }
            *dst++ = ':';
            *dst++ = ' ';
            dst = text_formatInt(item.d.prefixIndex, dst);
            while (*dst != '\0')
            {
                dst++;
            }
            *dst++ = ' ';
            dst = text_appendString(dst, TEXT_ID_ARROWS);
        }
    }

    truncateNameWithEllipsis(dst, nameStart, maxWidth);

    if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_ANCIENT)
    {
        setItemNameColor(playerIndex, HUD_TEXT_COLOR_GAMEPLAY_BLUE, HUD_TEXT_COLOR_MENU_BLUE);
    }
    return nameStart;
}
