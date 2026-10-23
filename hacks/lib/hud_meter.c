#include "lib/hud_meter.h"
#include "gba_io.h"

#define TILE_ROWS 8
#define TILE_PIXELS 8
#define TILE_WORDS 8
#define FILL_FIRST_ROW 1
#define FILL_LAST_ROW 5
#define TRIM_ROW 6
#define BASE_ROW 7
#define MAP_WIDTH 32

const HackLibHudMeter HackLibHudGapMeter = {
    .x = HACKLIB_HUD_GAP_X,
    .y = HACKLIB_HUD_GAP_Y,
    .widthTiles = HACKLIB_HUD_GAP_WIDTH,
    .firstTile = HACKLIB_HUD_FREE_TILE,
};

static u32 fillColor(const HackLibHudMeterLook *look, u32 px, u32 row)
{
    if (row == FILL_FIRST_ROW || row == FILL_LAST_ROW)
    {
        return px & 1 ? look->mid : look->dark;
    }
    return px & 1 ? look->light : look->mid;
}

static u32 pixelColor(const HackLibHudMeterLook *look, u32 px, u32 row, u32 width, u32 fill,
                      const u8 *markPx, u32 markCount)
{
    u32 i;

    if (row == BASE_ROW)
    {
        return HACKLIB_HUD_COLOR_NEAR_BLACK;
    }
    if (row == 0 || row == TRIM_ROW || px == 0 || px == width - 1)
    {
        return row == 0 ? HACKLIB_HUD_COLOR_GOLD : HACKLIB_HUD_COLOR_DARK_GOLD;
    }
    for (i = 0; i < markCount; i++)
    {
        if (px == markPx[i] && (row == FILL_FIRST_ROW || row == FILL_LAST_ROW))
        {
            return look->mark;
        }
    }
    if (px > fill)
    {
        return HACKLIB_HUD_COLOR_NEAR_BLACK;
    }
    return fillColor(look, px, row);
}

static void drawTiles(const HackLibHudMeter *meter,
                      u32 (*color)(const void *context, u32 px, u32 row), const void *context)
{
    u32 cnt = REG_BG0CNT;
    volatile u32 *tiles =
        (volatile u32 *)(BG_CHAR_ADDR(BGCNT_GET_CHARBASE(cnt)) + meter->firstTile * 32);
    volatile u16 *map =
        (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(cnt)) + meter->y * MAP_WIDTH + meter->x;
    u32 t;

    for (t = 0; t < meter->widthTiles; t++)
    {
        u32 row;

        for (row = 0; row < TILE_ROWS; row++)
        {
            u32 word = 0;
            u32 x;

            for (x = 0; x < TILE_PIXELS; x++)
            {
                word |= color(context, t * TILE_PIXELS + x, row) << (x * 4);
            }
            tiles[t * TILE_WORDS + row] = word;
        }
        map[t] = meter->firstTile + t;
    }
}

typedef struct SegmentDraw
{
    const HackLibHudMeterLook *const *looks;
    u32 count;
    u32 width;

    // interior pixels per segment
    u32 segmentWidth;

    // lit pixels per filled segment
    u32 fill;
} SegmentDraw;

static u32 segmentColor(const void *context, u32 px, u32 row)
{
    const SegmentDraw *draw = context;
    u32 segment;
    u32 offset;

    if (row == BASE_ROW)
    {
        return HACKLIB_HUD_COLOR_NEAR_BLACK;
    }
    if (row == 0 || row == TRIM_ROW || px == 0 || px >= draw->width - 1)
    {
        return row == 0 ? HACKLIB_HUD_COLOR_GOLD : HACKLIB_HUD_COLOR_DARK_GOLD;
    }
    segment = (px - 1) / (draw->segmentWidth + 1);
    offset = (px - 1) % (draw->segmentWidth + 1);
    if (segment >= draw->count || offset == draw->segmentWidth)
    {
        return HACKLIB_HUD_COLOR_DARK_GOLD;
    }
    if (draw->looks[segment] == NULL || offset >= draw->fill)
    {
        return HACKLIB_HUD_COLOR_NEAR_BLACK;
    }
    return fillColor(draw->looks[segment], px, row);
}

void hackLib_drawHudSegments(const HackLibHudMeter *meter, const HackLibHudMeterLook *const *looks,
                             u32 count, s32 value, s32 max)
{
    SegmentDraw draw = {.looks = looks, .count = count, .width = meter->widthTiles * TILE_PIXELS};

    if (count == 0 || max <= 0)
    {
        return;
    }
    if (value < 0)
    {
        value = 0;
    }
    if (value > max)
    {
        value = max;
    }
    draw.segmentWidth = (draw.width - 2 - (count - 1)) / count;
    // A segment with any time left keeps at least one lit pixel.
    draw.fill = value == 0 ? 0 : 1 + (u32)(value - 1) * draw.segmentWidth / (u32)max;
    drawTiles(meter, segmentColor, &draw);
}

void hackLib_drawHudMeter(const HackLibHudMeter *meter, s32 value, s32 max,
                          const HackLibHudMeterLook *look, const u8 *marks, u32 markCount)
{
    u32 cnt = REG_BG0CNT;
    volatile u32 *tiles =
        (volatile u32 *)(BG_CHAR_ADDR(BGCNT_GET_CHARBASE(cnt)) + meter->firstTile * 32);
    volatile u16 *map =
        (volatile u16 *)BG_SCREEN_ADDR(BGCNT_GET_SCREENBASE(cnt)) + meter->y * MAP_WIDTH + meter->x;
    u32 width = meter->widthTiles * TILE_PIXELS;
    u32 inner = width - 2;
    u8 markPx[4];
    u32 fill;
    u32 t;
    u32 i;

    if (max <= 0)
    {
        return;
    }
    if (value < 0)
    {
        value = 0;
    }
    if (value > max)
    {
        value = max;
    }
    // Interior pixels run 1..inner, so a fill of 0 draws nothing.
    fill = (u32)value * inner / (u32)max;
    if (markCount > ARRAY_COUNT(markPx))
    {
        markCount = ARRAY_COUNT(markPx);
    }
    for (i = 0; i < markCount; i++)
    {
        markPx[i] = 1 + marks[i] * inner / (u32)max;
    }
    for (t = 0; t < meter->widthTiles; t++)
    {
        u32 row;

        for (row = 0; row < TILE_ROWS; row++)
        {
            u32 word = 0;
            u32 x;

            for (x = 0; x < TILE_PIXELS; x++)
            {
                word |= pixelColor(look, t * TILE_PIXELS + x, row, width, fill, markPx, markCount)
                        << (x * 4);
            }
            tiles[t * TILE_WORDS + row] = word;
        }
        map[t] = meter->firstTile + t;
    }
}
