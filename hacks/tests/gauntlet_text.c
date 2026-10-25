#include "font.h"
#include "patches/gauntlet/gauntlet_ui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FontDesc g_FontDescriptors[5];
FontRenderCtx g_FontRenderCtx;
FontTemplateArg g_FontTemplateArgs[4];
const FontStyleInfo FontStyleInfos[5] = {0};

void *memory_malloc(size_t size)
{
    return malloc(size);
}
void *memory_memset(void *destination, u32 value, size_t count)
{
    return memset(destination, value, count);
}
char *text_formatInt(s32 value, char *destination)
{
    sprintf(destination, "%d", value);
    return destination;
}

static u16 screen[32 * 32];
static u32 tiles[512 * 8];
static u16 offsets[] = {0};
static u8 widths[] = {3};
static u8 glyph[] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55};

static u32 pixel(u32 x, u32 y)
{
    u32 tile = screen[(y / 8) * 32 + x / 8] & 1023;
    return (tiles[tile * 8 + y % 8] >> ((x % 8) * 4)) & 15;
}

static void reset(void)
{
    memset(screen, 0, sizeof(screen));
    memset(tiles, 0, sizeof(tiles));
    g_FontDescriptors[FONT_STYLE_8PX_B] = (FontDesc){.height = 8,
                                                     .firstChar = 'A',
                                                     .lastChar = 'A',
                                                     .lineHeight = 9,
                                                     .glyphOffsets = offsets,
                                                     .glyphWidths = widths,
                                                     .glyphBitmaps = glyph};
    font_setRenderTarget(screen, (u8 *)tiles, FALSE);
}

static u32 readWord(const u8 *bytes)
{
    return bytes[0] | bytes[1] << 8 | bytes[2] << 16 | (u32)bytes[3] << 24;
}

static u32 assetOffset(const u8 *bytes)
{
    return bytes[1] << 16 | bytes[2] << 8 | bytes[3];
}

static void checkCardBudget(const char *romPath, u32 styleTable)
{
    FILE *file = fopen(romPath, "rb");
    assert(file != NULL);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    u8 *rom = malloc(size);
    assert(rom != NULL && fread(rom, 1, size, file) == (size_t)size);
    fclose(file);
    for (u32 style = 0; style < 5; style++)
    {
        const u8 *info = rom + styleTable + style * 8;
        const u8 *asset = rom + readWord(info) - 0x08000000;
        g_FontDescriptors[style] =
            (FontDesc){.height = asset[2],
                       .firstChar = asset[0],
                       .lastChar = asset[1],
                       .lineHeight = readWord(info + 4),
                       .glyphOffsets = (u16 *)(asset + assetOffset(asset + 4)),
                       .glyphWidths = (u8 *)(asset + assetOffset(asset + 8)),
                       .glyphBitmaps = (u8 *)(asset + assetOffset(asset + 12))};
    }
    memset(screen, 0, sizeof(screen));
    memset(tiles, 0, sizeof(tiles));
    font_setRenderTarget(screen, (u8 *)tiles, FALSE);
    font_setStyle(FONT_STYLE_11PX_A, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    font_setStyle(FONT_STYLE_8PX_B, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    s32 tile = font_drawString(2, 12, 8, "CHOOSE A DOOR", 0);
    tile = font_drawString(tile, 228, 5, "Room 999", 2);
    tile = font_drawString(tile, 140, 5, "Select:Shop L:Goals", 1);
    u32 hintWidth = font_getStringWidth("Select:Shop L:Goals");
    assert(140 - hintWidth / 2 >= 12 + font_getStringWidth("CHOOSE A DOOR"));
    assert(140 + (hintWidth + 1) / 2 <= 228 - font_getStringWidth("Room 999"));
    font_setStyle(FONT_STYLE_8PX_B, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    for (u32 slot = 0; slot < 3; slot++)
    {
        tile = font_drawString(tile, 44 + slot * 72, 24, "Onslaught", 1);
        tile = font_drawString(tile, 44 + slot * 72, 96, "Rare boon", 1);
        tile = font_drawString(tile, 44 + slot * 72, 104, "Armored", 1);
    }
    char *description = "Gorgoroth Hills: champions of the Men of the South. Three waves. "
                        "Wave breaks 1/3 as long. Rare boon priority. +1 boon reroll.";
    font_setLineHeight(8);
    tile = font_drawLinesInBox(tile, 20, 128, 200, 24, &description, 0);
    assert(*description == '\0');
    assert(tile <= 320);
    printf("PASS: real-font door layout fits its panel and tile budget (%d/320 tiles)\n", tile);
    font_setRenderTarget(screen, (u8 *)tiles, FALSE);
    s32 goalsTile = font_drawString(2, 120, 8, "BOON GOALS", 1);
    for (u32 i = 0; i < 2; i++)
    {
        s32 y = 32 + i * 48;
        goalsTile = font_drawString(goalsTile, 20, y,
            i == 0 ? "Apprentice's Oath" : "Veteran's Purse", 0);
        goalsTile = font_drawString(goalsTile, 20, y + 12,
            i == 0 ? "Not owned: 12/12 paid casts" : "Not owned: 30/30 kills", 0);
        goalsTile = font_drawString(goalsTile, 20, y + 24,
            i == 0 ? "Spirit refund: 10% -> 30%" : "Extra gems per kill: 1 -> 3", 0);
    }
    goalsTile = font_drawString(goalsTile, 120, 144, "B / L: back to doors", 1);
    assert(goalsTile <= 320);
    printf("PASS: boon goals fit text budget (%d/320 tiles)\n", goalsTile);
    char *shrine = "No fight. Gain random equipment, scaled to run depth. "
                   "Equipped or stored in your pack. Full pack: 1 boon reroll instead.";
    tile = font_drawLinesInBox(2, 20, 128, 200, 24, &shrine, 0);
    assert(*shrine == '\0');
    assert(tile <= 320);
    memset(screen, 0, sizeof(screen));
    memset(tiles, 0, sizeof(tiles));
    font_setRenderTarget(screen, (u8 *)tiles, FALSE);
    font_setStyle(FONT_STYLE_11PX_A, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(2, 12, 5, "MERCHANT", 0);
    font_setStyle(FONT_STYLE_8PX_B, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(tile, 222, 8, "65535", 2);
    tile = font_drawString(tile, 64, 28, "> BUY <", 1);
    tile = font_drawString(tile, 176, 28, "SELL", 1);
    char longName[40] = {0};
    for (u32 count = 0; count < sizeof(longName) - 2; count++)
    {
        longName[count] = 'W';
        if (font_getStringWidth(longName) > GAUNTLET_UI_SHOP_NAME_WIDTH)
        {
            longName[count] = 0;
            break;
        }
    }
    for (u32 row = 0; row < 4; row++)
    {
        s32 y = 40 + row * 24;
        tile = font_drawString(tile, 48, y + 4, longName, 0);
        tile = font_drawString(tile, 48, y + 12, "GREATSWORD", 0);
        tile = font_drawString(tile, 222, y + 4, "65535g", 2);
        tile = font_drawString(tile, 222, y + 12, "Lv 50", 2);
    }
    tile = font_drawString(tile, 20, 140, "[A]", 0);
    tile = font_drawString(tile, 38, 140, "Inspect", 0);
    tile = font_drawString(tile, 126, 140, "[B]", 0);
    tile = font_drawString(tile, 144, 140, "Doors", 0);
    tile = font_drawString(tile, 120, 148, "Item changed or cannot be sold.", 1);
    printf("Shop worst-case text budget: %d/320 tiles\n", tile);
    fflush(stdout);
    assert(tile <= 320);
    memset(screen, 0, sizeof(screen));
    memset(tiles, 0, sizeof(tiles));
    font_setRenderTarget(screen, (u8 *)tiles, FALSE);
    font_setStyle(FONT_STYLE_11PX_A, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(2, 12, 5, "SELL GEAR?", 0);
    font_setStyle(FONT_STYLE_8PX_B, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(tile, 222, 8, "65535", 2);
    tile = font_drawString(tile, 120, 24, "65535 gems  /  Lv 50  /  After 65535", 1);
    char line[40] = {0};
    for (u32 count = 0; count < sizeof(line) - 2; count++)
    {
        line[count] = 'W';
        if (font_getStringWidth(line) > 92)
        {
            line[count] = 0;
            break;
        }
    }
    char names[100];
    char stats[200];
    snprintf(names, sizeof(names), "%s\n%s", line, line);
    snprintf(stats, sizeof(stats), "%s\n%s\n%s\n%s", line, line, line, line);
    for (u32 column = 0; column < 2; column++)
    {
        s32 x = column == 0 ? 18 : 130;
        tile = font_drawString(tile, column == 0 ? 64 : 176, 40, "EQUIPPED", 1);
        char *remaining = names;
        tile = font_drawLinesInBox(tile, x, 76, 92, 16, &remaining, 0);
        assert(*remaining == '\0');
        remaining = stats;
        tile = font_drawLinesInBox(tile, x, 96, 92, 32, &remaining, 0);
        assert(*remaining == '\0');
    }
    tile = font_drawString(tile, 20, 140, "[A]", 0);
    tile = font_drawString(tile, 38, 140, "Confirm sale", 0);
    tile = font_drawString(tile, 126, 140, "[B]", 0);
    tile = font_drawString(tile, 144, 140, "Cancel", 0);
    tile = font_drawString(tile, 120, 148, "Up/Down: browse   L/R: stats", 1);
    printf("Comparison worst-case text budget: %d/320 tiles\n", tile);
    fflush(stdout);
    assert(tile <= 320);
    free(rom);
}

int main(int argc, char **argv)
{
    // Reproduce adjacent card labels losing pixels even though their glyphs do not overlap.
    reset();
    font_setStyle(FONT_STYLE_8PX_B, 0, 0);
    s32 tile = font_drawString(1, 0, 82, "A", 0);
    assert(pixel(0, 89) == 1);
    font_setStyle(FONT_STYLE_8PX_B, 3, 0);
    font_drawString(tile, 0, 94, "A", 0);
    assert(pixel(0, 89) == 0);

    // Preserve each label and its color when both occupy part of the same screen tile.
    reset();
    font_setStyle(FONT_STYLE_8PX_B, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    tile = font_drawString(1, 0, 82, "A", 0);
    font_setStyle(FONT_STYLE_8PX_B, 3, GAUNTLET_UI_TEXT_BACKGROUND);
    font_drawString(tile, 0, 94, "A", 0);
    for (u32 y = 82; y < 90; y++)
    {
        assert(pixel(0, y) == 1);
    }
    for (u32 y = 94; y < 102; y++)
    {
        assert(pixel(0, y) == 4);
    }

    // Compact paragraph lines must retain the bottom pixels of earlier lines too.
    reset();
    font_setStyle(FONT_STYLE_8PX_B, 0, GAUNTLET_UI_TEXT_BACKGROUND);
    font_setLineHeight(9);
    font_drawLines(1, 0, 2, 80, "A\nA\nA", 0);
    for (u32 line = 0; line < 3; line++)
    {
        for (u32 y = 2 + line * 9; y < 10 + line * 9; y++)
        {
            assert(pixel(0, y) == 1);
        }
    }
    puts("PASS: card labels and compact paragraphs preserve neighboring glyph pixels");
    if (argc == 3)
    {
        checkCardBudget(argv[1], strtoul(argv[2], NULL, 0));
    }
    return 0;
}
