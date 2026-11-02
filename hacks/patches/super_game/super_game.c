#include "patches/super_game/super_game.h"

#include "font.h"
#include "gfx.h"
#include "lib/pc_host.h"
#include "menu.h"
#include "player.h"
#include "scene.h"
#include "scene/mainMenu.h"
#include "sfx.h"
#include "text.h"
#include "variables.h"

#if PC_EDITION

#define SUPER_GAME_HOST_VERSION 5
#define SUPER_GAME_ROW_TEXT "The Two Towers"

// The stock rows' layout (scene_mainMenu_drawEntry): 30 BG0 tiles a row from tile 1, centered at
// x 120, 8 pixels apart from y 96.
#define MAIN_MENU_ROW_TILES 0x1e
#define MAIN_MENU_ROW_X 0x78
#define MAIN_MENU_ROW_Y 0x60
#define MAIN_MENU_ROW_HEIGHT 8
#define MAIN_MENU_COLOR_SELECTED 0
#define MAIN_MENU_COLOR 3
// The text band scene_mainMenu_drawEntries clears: tile rows 12..19, all 30 columns.
#define MAIN_MENU_BAND_ROW 0xc
#define MAIN_MENU_BAND_WIDTH 0x1e
#define MAIN_MENU_BAND_HEIGHT 8
#define FONT_ALIGN_CENTER 1

static volatile PcHostInfo *superGameHost(void)
{
    volatile PcHostInfo *info = hackLib_pcHostInfoWritable();

    return info != NULL && info->version >= SUPER_GAME_HOST_VERSION ? info : NULL;
}

u32 superGame_mainMenuRowCount(void)
{
    volatile PcHostInfo *info = superGameHost();

    return info != NULL && (info->superGame & PC_HOST_SUPER_GAME_AVAILABLE) ? MAIN_MENU_ENTRY_COUNT + 1
                                                                             : MAIN_MENU_ENTRY_COUNT;
}

void superGame_mainMenuConfirm(void)
{
    volatile PcHostInfo *info = superGameHost();

    if (info == NULL || !(info->superGame & PC_HOST_SUPER_GAME_AVAILABLE))
    {
        return;
    }
    g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
    info->superGame |= PC_HOST_SUPER_GAME_SWITCH;
}

static char *rowText(u32 row)
{
    return row == SUPER_GAME_MAIN_MENU_ROW ? SUPER_GAME_ROW_TEXT : text_getString(TEXT_ID_MAIN_MENU_ENTRY_BASE + row);
}

static void drawRow(u32 row, u32 selected)
{
    font_setStyle(FONT_STYLE_8PX_B, row == selected ? MAIN_MENU_COLOR_SELECTED : MAIN_MENU_COLOR, 0);
    font_drawString(row * MAIN_MENU_ROW_TILES + 1, MAIN_MENU_ROW_X, MAIN_MENU_ROW_Y + row * MAIN_MENU_ROW_HEIGHT,
                    rowText(row), FONT_ALIGN_CENTER);
}

/**
 * One main menu row. The redraw loop in scene_mainMenu_tick stops at the stock six, so the last of
 * them also draws The Two Towers.
 *
 * REPLACES (whole function): scene_mainMenu_drawEntry.
 */
void superGame_drawEntry(s32 entryIndex, s32 selectedIndex)
{
    drawRow(entryIndex, selectedIndex);
    if (entryIndex == MAIN_MENU_ENTRY_BONUS_MAPS && superGame_mainMenuRowCount() > MAIN_MENU_ENTRY_COUNT)
    {
        drawRow(SUPER_GAME_MAIN_MENU_ROW, selectedIndex);
    }
}

/**
 * Every main menu row, the selected one highlighted, then back to idle.
 *
 * REPLACES (whole function): scene_mainMenu_drawEntries.
 */
void superGame_drawEntries(void)
{
    gfx_fillBgRect(0, 0, MAIN_MENU_BAND_ROW, MAIN_MENU_BAND_WIDTH, MAIN_MENU_BAND_HEIGHT, 0);
    for (u32 row = 0; row < superGame_mainMenuRowCount(); row++)
    {
        drawRow(row, SCENE_ARG_MAIN_MENU_CURSOR);
    }
    SCENE_ARG_MAIN_MENU_STATE = MAIN_MENU_STATE_IDLE;
    if (sfx_isPlaying(g_MainMenuSfxHandle))
    {
        sfx_stop(g_MainMenuSfxHandle);
    }
    g_MainMenuSfxHandle = sfx_play(SFX_MENU_CONFIRM);
}

/**
 * D-pad on the main menu: the cursor wraps over every row, The Two Towers' included.
 *
 * REPLACES (whole function): scene_mainMenu_moveCursor.
 */
void superGame_moveCursor(void)
{
    u32 previous = SCENE_ARG_MAIN_MENU_CURSOR;

    menu_movePlayerCursorVertical(&SCENE_ARG_MAIN_MENU_CURSOR, 0, superGame_mainMenuRowCount() - 1, TRUE,
                                  g_PlayerGlobals.activePlayerIndex);
    drawRow(previous, SCENE_ARG_MAIN_MENU_CURSOR);
    drawRow(SCENE_ARG_MAIN_MENU_CURSOR, SCENE_ARG_MAIN_MENU_CURSOR);
    if (sfx_isPlaying(g_MainMenuSfxHandle))
    {
        sfx_stop(g_MainMenuSfxHandle);
    }
    g_MainMenuSfxHandle = sfx_play(SFX_MENU_CURSOR_MOVE);
}

#endif
