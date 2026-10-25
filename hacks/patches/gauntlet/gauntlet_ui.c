#include "patches/gauntlet/gauntlet_ui.h"

#include "font.h"
#include "gba_io.h"
#include "gfx.h"
#include "variables.h"

#define BG_PANEL 1u
// Charbase 0 holds the backdrop art's tiles below PANEL_TILE_BASE. Screenbase 2 lies past them.
#define PANEL_BGCNT (BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(2u))
#define PANEL_TILE_BASE 192u
#define BG_TEXT 2u
// The first pixel row of the dialog box drawn behind a dialog portrait's speaker.
#define PORTRAIT_BOX_TOP 24
// Palette banks. The backdrop art uses bank 3 and the UI text bank 0.
#define PANEL_BANK 12u
#define PORTRAIT_BANK 13u
#define MAP_CELLS 1024u
#define MAP_WIDTH 32
#define TEXT_TILE_COUNT 320u
#define HIGHLIGHT_STEPS 6u

// The front backdrop ends at charblock 2's tile 0. Keep it intact in both text pages.
static const u16 TextControls[] = {
    GAUNTLET_UI_TEXT_BGCNT,
    BGCNT_PRIORITY(1) | BGCNT_CHARBASE(2) | BGCNT_SCREENBASE(0x16u),
};
static const u16 PanelControls[] = {
    PANEL_BGCNT,
    BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(4u),
};
static u8 visiblePage;
static u8 drawPage;
static u8 highlightStep;

enum PanelColor
{
    PANEL_CLEAR,
    PANEL_FILL,
    PANEL_FILL_SHADE,
    PANEL_HIGHLIGHT,
    PANEL_HIGHLIGHT_EDGE,
    PANEL_GOLD,
    PANEL_GOLD_DARK,
    PANEL_GEM_DARK,
    PANEL_GEM,
    PANEL_GEM_LIGHT,
    PANEL_COLOR_COUNT,
};

#define RGB5(r, g, b) ((r) | (g) << 5 | (b) << 10)

static const u16 PanelColors[PANEL_COLOR_COUNT] = {
    [PANEL_CLEAR] = 0,
    [PANEL_FILL] = RGB5(3, 2, 2),
    [PANEL_FILL_SHADE] = RGB5(5, 4, 3),
    [PANEL_HIGHLIGHT] = RGB5(12, 8, 2),
    [PANEL_HIGHLIGHT_EDGE] = RGB5(20, 14, 4),
    [PANEL_GOLD] = RGB5(30, 25, 11),
    [PANEL_GOLD_DARK] = RGB5(17, 12, 5),
    [PANEL_GEM_DARK] = RGB5(2, 9, 7),
    [PANEL_GEM] = RGB5(5, 23, 15),
    [PANEL_GEM_LIGHT] = RGB5(22, 31, 23),
};

enum PanelTile
{
    TILE_BLANK,
    TILE_FILL,
    TILE_HIGHLIGHT,
    TILE_TOP_LEFT,
    TILE_TOP,
    TILE_TOP_RIGHT,
    TILE_LEFT,
    TILE_RIGHT,
    TILE_BOTTOM_LEFT,
    TILE_BOTTOM,
    TILE_BOTTOM_RIGHT,
    TILE_SELECTED_FILL,
    TILE_SELECTED_TOP_LEFT,
    TILE_SELECTED_TOP,
    TILE_SELECTED_TOP_RIGHT,
    TILE_SELECTED_LEFT,
    TILE_SELECTED_RIGHT,
    TILE_SELECTED_BOTTOM_LEFT,
    TILE_SELECTED_BOTTOM,
    TILE_SELECTED_BOTTOM_RIGHT,
    TILE_GEM,
    PANEL_TILE_COUNT,
};

/** Fill pixel color, a soft vertical shade so the panel is not flat. */
static u32 fillColor(s32 y)
{
    return (y & 3) == 0 ? PANEL_FILL_SHADE : PANEL_FILL;
}

/**
 * One pixel of a border tile. edgeX / edgeY say which side is the outside: -1 left or top,
 * 1 right or bottom, 0 none. The frame is a gold line with a dark gold line inside it.
 */
static u32 borderPixel(s32 x, s32 y, s32 edgeX, s32 edgeY)
{
    s32 dx = edgeX < 0 ? x : edgeX > 0 ? 7 - x : 8;
    s32 dy = edgeY < 0 ? y : edgeY > 0 ? 7 - y : 8;
    s32 d = dx < dy ? dx : dy;

    // Round the outer corner off.
    if (dx == 0 && dy == 0)
    {
        return PANEL_CLEAR;
    }
    if (d == 0)
    {
        return PANEL_GOLD;
    }
    if (d == 1)
    {
        return PANEL_GOLD_DARK;
    }
    return fillColor(y);
}

static u32 tilePixel(u32 tile, s32 x, s32 y)
{
    if (tile >= TILE_SELECTED_FILL && tile <= TILE_SELECTED_BOTTOM_RIGHT)
    {
        static const u8 Bases[] = {TILE_FILL,        TILE_TOP_LEFT, TILE_TOP,
                                   TILE_TOP_RIGHT,   TILE_LEFT,     TILE_RIGHT,
                                   TILE_BOTTOM_LEFT, TILE_BOTTOM,   TILE_BOTTOM_RIGHT};
        u32 pixel = tilePixel(Bases[tile - TILE_SELECTED_FILL], x, y);
        return pixel == PANEL_FILL || pixel == PANEL_FILL_SHADE
                   ? ((y & 3) == 0 ? PANEL_HIGHLIGHT_EDGE : PANEL_HIGHLIGHT)
                   : pixel;
    }
    switch (tile)
    {
    case TILE_FILL:
        return fillColor(y);
    case TILE_HIGHLIGHT:
        return y == 0 || y == 7 ? PANEL_HIGHLIGHT_EDGE : PANEL_HIGHLIGHT;
    case TILE_GEM: {
        s32 dx = x > 3 ? x - 4 : 3 - x;
        s32 dy = y > 3 ? y - 4 : 3 - y;
        if (dx + dy > 3)
        {
            return PANEL_FILL;
        }
        return x < 4 && y < 4 ? PANEL_GEM_LIGHT : x > y ? PANEL_GEM : PANEL_GEM_DARK;
    }
    case TILE_TOP_LEFT:
        return borderPixel(x, y, -1, -1);
    case TILE_TOP:
        return borderPixel(x, y, 0, -1);
    case TILE_TOP_RIGHT:
        return borderPixel(x, y, 1, -1);
    case TILE_LEFT:
        return borderPixel(x, y, -1, 0);
    case TILE_RIGHT:
        return borderPixel(x, y, 1, 0);
    case TILE_BOTTOM_LEFT:
        return borderPixel(x, y, -1, 1);
    case TILE_BOTTOM:
        return borderPixel(x, y, 0, 1);
    case TILE_BOTTOM_RIGHT:
        return borderPixel(x, y, 1, 1);
    default:
        return PANEL_CLEAR;
    }
}

/** Write the panel tiles into VRAM, one 4bpp row (a word) at a time. */
static void buildTiles(void)
{
    volatile u32 *dst = (volatile u32 *)(BG_CHAR_ADDR(0) + PANEL_TILE_BASE * 32);
    u32 tile;
    s32 x;
    s32 y;

    for (tile = 0; tile < PANEL_TILE_COUNT; tile++)
    {
        for (y = 0; y < 8; y++)
        {
            u32 row = 0;

            for (x = 0; x < 8; x++)
            {
                row |= tilePixel(tile, x, y) << (x * 4);
            }
            *dst++ = row;
        }
    }
}

static volatile u16 *map(void)
{
    return (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(PanelControls[drawPage]));
}

static void setCell(s32 x, s32 y, u32 tile)
{
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_WIDTH)
    {
        map()[y * MAP_WIDTH + x] = tile | PANEL_BANK << 12;
    }
}

void gauntletUi_init(void)
{
    visiblePage = drawPage = 0;
    highlightStep = HIGHLIGHT_STEPS;
    gfx_setupBg(BG_PANEL, PANEL_BGCNT);
    gfx_setBgOffset(BG_PANEL, 0);
    gfx_loadBgPaletteColors(PanelColors, PANEL_BANK * 16, PANEL_COLOR_COUNT);
    buildTiles();
    gauntletUi_clear();
}

void gauntletUi_beginFrame(void)
{
    volatile u16 blank = 1;
    volatile u16 zero = 0;
    u32 control;

    drawPage = visiblePage ^ 1;
    control = TextControls[drawPage];
    font_setBgTarget(control);
    DMA3_FILL16(&zero, BG_CHAR_ADDR(BGCNT_GET_CHARBASE(control)) + 32, (TEXT_TILE_COUNT - 1) * 16);
    DMA3_FILL16(&blank, BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(control)), MAP_CELLS);
    gauntletUi_clear();
}

void gauntletUi_present(void)
{
    while (REG_VCOUNT < 160)
    {
    }
    gfx_setBgControl(BG_PANEL, PanelControls[drawPage]);
    gfx_setBgControl(BG_TEXT, TextControls[drawPage]);
    visiblePage = drawPage;
    highlightStep = 0;
}

void gauntletUi_tick(void)
{
    u16 colors[2];
    u32 color;
    u32 component;

    if (highlightStep == HIGHLIGHT_STEPS)
    {
        return;
    }
    highlightStep++;
    for (color = 0; color < ARRAY_COUNT(colors); color++)
    {
        colors[color] = 0;
        for (component = 0; component < 15; component += 5)
        {
            s32 start = (PanelColors[PANEL_FILL] >> component) & 31;
            s32 target = (PanelColors[PANEL_HIGHLIGHT + color] >> component) & 31;

            colors[color] |= (start + (target - start) * highlightStep / HIGHLIGHT_STEPS)
                             << component;
        }
    }
    gfx_loadBgPaletteColors(colors, PANEL_BANK * 16 + PANEL_HIGHLIGHT, ARRAY_COUNT(colors));
}

void gauntletUi_clear(void)
{
    volatile u16 zero = 0;

    DMA3_FILL16(&zero, map(), MAP_CELLS);
}

void gauntletUi_panel(s32 x, s32 y, s32 w, s32 h)
{
    s32 right = x + w - 1;
    s32 bottom = y + h - 1;
    s32 col;
    s32 row;

    for (row = y; row <= bottom; row++)
    {
        for (col = x; col <= right; col++)
        {
            u32 tile = TILE_FILL;

            if (row == y)
            {
                tile = col == x ? TILE_TOP_LEFT : col == right ? TILE_TOP_RIGHT : TILE_TOP;
            }
            else if (row == bottom)
            {
                tile = col == x ? TILE_BOTTOM_LEFT : col == right ? TILE_BOTTOM_RIGHT : TILE_BOTTOM;
            }
            else if (col == x)
            {
                tile = TILE_LEFT;
            }
            else if (col == right)
            {
                tile = TILE_RIGHT;
            }
            setCell(col, row, PANEL_TILE_BASE + tile);
        }
    }
}

void gauntletUi_highlight(s32 x, s32 y, s32 w)
{
    s32 col;

    for (col = x; col < x + w; col++)
    {
        setCell(col, y, PANEL_TILE_BASE + TILE_HIGHLIGHT);
    }
}

void gauntletUi_card(s32 x, s32 y, s32 w, s32 h, bool selected)
{
    static const u8 Selected[] = {TILE_BLANK,
                                  TILE_SELECTED_FILL,
                                  TILE_SELECTED_FILL,
                                  TILE_SELECTED_TOP_LEFT,
                                  TILE_SELECTED_TOP,
                                  TILE_SELECTED_TOP_RIGHT,
                                  TILE_SELECTED_LEFT,
                                  TILE_SELECTED_RIGHT,
                                  TILE_SELECTED_BOTTOM_LEFT,
                                  TILE_SELECTED_BOTTOM,
                                  TILE_SELECTED_BOTTOM_RIGHT};
    gauntletUi_panel(x, y, w, h);
    if (selected)
    {
        for (s32 row = y; row < y + h; row++)
        {
            for (s32 col = x; col < x + w; col++)
            {
                if (row < 0 || row >= MAP_WIDTH || col < 0 || col >= MAP_WIDTH)
                {
                    continue;
                }
                u32 tile = (map()[row * MAP_WIDTH + col] & 0x3ff) - PANEL_TILE_BASE;
                setCell(col, row, PANEL_TILE_BASE + Selected[tile]);
            }
        }
    }
}

void gauntletUi_gem(s32 x, s32 y)
{
    setCell(x, y, PANEL_TILE_BASE + TILE_GEM);
}

static const u8 *portraitAsset(u32 heroId)
{
    static const u8 *const Portraits[] = {
        DialogPortraitFrodoBgAsset,   DialogPortraitLegolasBgAsset, DialogPortraitAragornBgAsset,
        DialogPortraitGandalfBgAsset, DialogPortraitEowynBgAsset,   DialogPortraitGimliBgAsset,
        DialogPortraitSamBgAsset,     DialogPortraitSmeagolBgAsset, DialogPortraitWitchKingBgAsset,
        DialogPortraitSarumanBgAsset,
    };

    return heroId < ARRAY_COUNT(Portraits) ? Portraits[heroId] : Portraits[0];
}

static u32 portraitPixel(volatile u32 *tiles, s32 x, s32 y)
{
    return (tiles[((y / 8) * GAUNTLET_UI_PORTRAIT_W + x / 8) * 8 + y % 8] >> (x % 8 * 4)) & 0xf;
}

static void clearPortraitPixel(volatile u32 *tiles, s32 x, s32 y)
{
    volatile u32 *row = &tiles[((y / 8) * GAUNTLET_UI_PORTRAIT_W + x / 8) * 8 + y % 8];

    *row &= ~(0xfu << (x % 8 * 4));
}

/**
 * Dialog portraits carry the dialog box behind the speaker from PORTRAIT_BOX_TOP down: a line in
 * one color, then a checker of color 0 and a dark color. Walking in from both edges, clear box
 * pixels until the figure starts, so the panel shows around it.
 */
static void trimPortraitBox(volatile u32 *tiles)
{
    s32 width = GAUNTLET_UI_PORTRAIT_W * 8;
    u32 line = portraitPixel(tiles, 0, PORTRAIT_BOX_TOP);
    u32 dark = portraitPixel(tiles, 0, PORTRAIT_BOX_TOP + 1);
    s32 y;
    s32 x;

    for (y = PORTRAIT_BOX_TOP; y < GAUNTLET_UI_PORTRAIT_H * 8; y++)
    {
        u32 allowed = y == PORTRAIT_BOX_TOP ? line : dark;

        for (x = 0; x < width &&
                    (portraitPixel(tiles, x, y) == allowed || portraitPixel(tiles, x, y) == 0);
             x++)
        {
            clearPortraitPixel(tiles, x, y);
        }
        for (x = width - 1;
             x >= 0 && (portraitPixel(tiles, x, y) == allowed || portraitPixel(tiles, x, y) == 0);
             x--)
        {
            clearPortraitPixel(tiles, x, y);
        }
    }
}

void gauntletUi_portrait(u32 heroId, s32 x, s32 y)
{
    u32 control = TextControls[drawPage];
    u32 paletteBank = PORTRAIT_BANK + drawPage;
    const u8 *asset = portraitAsset(heroId);
    const u16 *palette = (const u16 *)(asset + 2);
    const u8 *stream = asset + 2 + 16 * sizeof(u16) + sizeof(u16);
    volatile u32 *tiles = (volatile u32 *)(BG_CHAR_ADDR(BGCNT_GET_CHARBASE(control)) +
                                           GAUNTLET_UI_PORTRAIT_TILE_BASE * 32);
    volatile u16 *textMap = (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(control));
    s32 col;
    s32 row;

    // Dialog portraits carry a 16-color palette and unmapped pucrunch tiles.
    gfx_loadBgPaletteColors(palette, paletteBank * 16, 16);
    ((void (*)(const void *, void *, u32))g_DecompressPucrunch)(stream, (void *)tiles, 0);
    trimPortraitBox(tiles);
    for (row = 0; row < GAUNTLET_UI_PORTRAIT_H; row++)
    {
        for (col = 0; col < GAUNTLET_UI_PORTRAIT_W; col++)
        {
            textMap[(y + row) * MAP_WIDTH + x + col] =
                (GAUNTLET_UI_PORTRAIT_TILE_BASE + row * GAUNTLET_UI_PORTRAIT_W + col) | paletteBank
                                                                                            << 12;
        }
    }
}
