#include "font.h"
#include "item.h"
#include "memory.h"
#include "player.h"
#include "text.h"
#include "variables.h"

/**
 * Draw an item's name and minimum level into an info panel at x/y.
 * Unique items with baseIndex < 26 use flavor text instead of accumulated stats.
 *
 * @param buffer Writable 200-byte scratch buffer.
 * @param y Initial name position, updated to 96 for the minimum-level line.
 * @param tileIndex Next free font tile index, advanced by drawing.
 * @return The description for the caller to draw, as unique flavor text or stats in buffer.
 *
 * @romaddress 0x0802c47c
 */
char *item_drawInfo(char *buffer, u32 playerIndex, Item item, const u16 *x, u16 *y, u16 *tileIndex)
{
    char *line = NULL;
    u8 minLevel;

    line = item_affix_formatName(buffer, item, 220);
    font_setStyle(FONT_STYLE_8PX_A, g_PlayerHuds[playerIndex].textColor, 0);
    *tileIndex = font_drawLinesInBox(*tileIndex, *x, *y, 118, 16, &line, 0);

    memory_memset(buffer, 0, 200);
    minLevel = item_getMinLevel(item.word);
    if (minLevel > PLAYER(playerIndex).level + 1)
    {
        font_setStyle(FONT_STYLE_8PX_A, 6, 0);
    }
    else
    {
        font_setStyle(FONT_STYLE_8PX_A, 9, 0);
    }
    *y = 96;
    line = text_formatInt(minLevel, buffer);
    font_setTemplateArg0(line);
    line = text_getString(TEXT_ID_MIN_LEVEL_1);
    *tileIndex = font_drawLines(*tileIndex, *x, *y, 118, line, 0);

    font_setStyle(FONT_STYLE_8PX_A, 3, 0);
    if (item.d.itemType == ITEM_TYPE_UNIQUE && item.d.baseIndex < 26)
    {
        line = text_getString(TEXT_ID_UNIQUE_DESC_BASE + item.d.baseIndex);
    }
    else
    {
        memory_memset(buffer, 0, 200);
        item_affix_accumulateStats(buffer, item);
        line = buffer;
    }

    return line;
}
