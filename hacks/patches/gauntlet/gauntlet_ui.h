#pragma once

// Framed panels, highlight bars and hero portraits for the Gauntlet scene, drawn on BG1 between
// the backdrop art and the text layer. Positions and sizes are in 8x8 cells.

#include "gba_io.h"
#include "types.h"

/** Set up BG1 with the panel tiles and palette. Call once in the scene's enter. */
void gauntletUi_init(void);

/** Prepare an invisible text and panel page for drawing. */
void gauntletUi_beginFrame(void);
/** Present the completed page during vertical blank. */
void gauntletUi_present(void);
/** Ease the selected row's highlight toward its resting color. */
void gauntletUi_tick(void);

/** Blank every BG1 cell. */
void gauntletUi_clear(void);

/** A framed dark panel covering w x h cells, border included. */
void gauntletUi_panel(s32 x, s32 y, s32 w, s32 h);

/** A highlight bar over one cell row, from x for w cells. Draw it inside a panel. */
void gauntletUi_highlight(s32 x, s32 y, s32 w);

/** A framed card with a warm fill when selected. */
void gauntletUi_card(s32 x, s32 y, s32 w, s32 h, bool selected);
void gauntletUi_gem(s32 x, s32 y);

/** The hero's portrait on the text layer, 7 x 8 cells, its top left cell at x, y. */
void gauntletUi_portrait(u32 heroId, s32 x, s32 y);

#define GAUNTLET_UI_PORTRAIT_W 7
#define GAUNTLET_UI_PORTRAIT_H 8

// Compact labels can share a screen tile without sharing glyph pixels.
#define GAUNTLET_UI_TEXT_BACKGROUND (-1)
#define GAUNTLET_UI_SHOP_NAME_WIDTH 96

// The scene's text layer (BG2), which also holds the portrait.
#define GAUNTLET_UI_TEXT_BGCNT (BGCNT_PRIORITY(1) | BGCNT_CHARBASE(3) | BGCNT_SCREENBASE(0x1du))
// Text tiles below this, the portrait from it. 320 tiles fit below the screenbases.
#define GAUNTLET_UI_PORTRAIT_TILE_BASE (320 - GAUNTLET_UI_PORTRAIT_W * GAUNTLET_UI_PORTRAIT_H)
