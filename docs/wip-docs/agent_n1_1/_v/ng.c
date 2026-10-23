#define scene_newGame_unknown08014634 hdr_dummy
#include "font.h"
#include "gfx.h"
#include "scene/newGame.h"
#include "text.h"
#include "variables.h"
#undef scene_newGame_unknown08014634

// Needs config: u8 *[] table at 0x082826e0 (per-page BG asset pointers).
#define NewGamePageAssets ((const u8 *const *)0x082826e0)

void scene_newGame_unknown08014634(void);

/**
 * Draw the current new game page: title and body text plus its picture and palette.
 *
 * @romaddress 0x08014634
 */
void scene_newGame_unknown08014634(void)
{
    char *str;

    font_setStyle(FONT_STYLE_11PX_A, 3, -1);
    str = text_getString(g_SceneCurrent.args[1] + 0x13);
    gfx_fillBgRect(0, 4, 0xe, 8, 2, 0);
    font_drawLines(0xd3, 0x45, 0x70, 0x40, str, 1);
    font_setStyle(FONT_STYLE_8PX_A, 3, -1);
    str = text_getString(g_SceneCurrent.args[1] + 0x1b);
    gfx_fillBgRect(0, 0, 0x10, 0xec, 3, 0);
    font_drawLines(0x1f, 2, 0x84, 0xee, str, 0);
    gfx_unknown080127d0(3, NewGamePageAssets[g_SceneCurrent.args[1]], 2, 0, 0x12, 4);
    gfx_loadBgPaletteColors((const u16 *)(NewGamePageAssets[g_SceneCurrent.args[1]] + 0x82), 0x40, 0xc0);
}
