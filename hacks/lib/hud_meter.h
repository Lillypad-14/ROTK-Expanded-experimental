#pragma once

#include "types.h"

// A small horizontal meter on the gameplay HUD, drawn into a strip of private BG tiles in the
// stock HP and Spirit bar style: gold frame on top, 5 rows of dithered fill, gold trim below.
// Redraw it every HUD tick. It keeps no state, so HUD reloads can't leave it stale.

// Gameplay HUD layer. Its char and screen blocks are read from BG0CNT at draw time.
#define HACKLIB_HUD_BG 0

// Gap between the HP and Spirit bars on HUD row 18, and 9 free tiles of the HUD char block.
#define HACKLIB_HUD_GAP_X 11
#define HACKLIB_HUD_GAP_Y 18
#define HACKLIB_HUD_GAP_WIDTH 8
#define HACKLIB_HUD_FREE_TILE 0x4d
#define HACKLIB_HUD_FREE_TILE_COUNT 9

// HUD palette bank 0 colors (the stock bars use these).
enum HackLibHudColor
{
    HACKLIB_HUD_COLOR_WHITE = 1,
    HACKLIB_HUD_COLOR_PALE_GOLD = 2,
    HACKLIB_HUD_COLOR_GOLD = 3,
    HACKLIB_HUD_COLOR_DARK_GOLD = 4,
    HACKLIB_HUD_COLOR_BROWN = 5,
    HACKLIB_HUD_COLOR_NEAR_BLACK = 6, // empty bar
    HACKLIB_HUD_COLOR_EMBER = 7,
    HACKLIB_HUD_COLOR_RED = 8,
    HACKLIB_HUD_COLOR_DARK_RED = 9,
    HACKLIB_HUD_COLOR_LIGHT_BLUE = 10,
    HACKLIB_HUD_COLOR_BLUE = 11,
    HACKLIB_HUD_COLOR_DARK_BLUE = 12,
    HACKLIB_HUD_COLOR_LIGHT_GREEN = 13,
    HACKLIB_HUD_COLOR_GREEN = 14,
    HACKLIB_HUD_COLOR_DARK_GREEN = 15,
};

typedef struct HackLibHudMeter
{
    // map position in tiles
    u8 x;
    u8 y;

    // width in tiles, at most the free tiles it owns
    u8 widthTiles;

    // first of widthTiles consecutive char tiles the meter owns
    u16 firstTile;
} HackLibHudMeter;

typedef struct HackLibHudMeterLook
{
    // fill highlight, mid and shadow colors (HackLibHudColor)
    u8 light;
    u8 mid;
    u8 dark;

    // color of the threshold notches
    u8 mark;
} HackLibHudMeterLook;

/** The meter between the HP and Spirit bars. */
extern const HackLibHudMeter HackLibHudGapMeter;

/**
 * Draw `value` out of `max`, with a notch at each of `marks` (values on the same scale).
 */
void hackLib_drawHudMeter(const HackLibHudMeter *meter, s32 value, s32 max,
                          const HackLibHudMeterLook *look, const u8 *marks, u32 markCount);

/**
 * Draw `count` equal segments split by trim dividers. Segment i fills `value` out of `max` of its
 * width with `looks[i]`, or stays empty when `looks[i]` is NULL.
 */
void hackLib_drawHudSegments(const HackLibHudMeter *meter, const HackLibHudMeterLook *const *looks,
                             u32 count, s32 value, s32 max);
