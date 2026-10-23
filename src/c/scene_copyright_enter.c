#include "font.h"
#include "gfx.h"
#include "scene/copyright.h"
#include "sfx.h"
#include "system.h"
#include "text.h"
#include "variables.h"

/**
 * Set up the copyright screen: backdrop, two text blocks, and the title song.
 *
 * @romaddress 0x0802fe5c
 */
void scene_copyright_enter(void)
{
    u32 textBg;
    s32 charbase = 0x100;

    gfx_initDisplay(0);
    gfx_setupBg(3, MenuBackdropBgCnt);
    gfx_loadBgAsset(3, CopyrightBg3Asset, 0, 1, 0, 0);
    textBg = MenuTextBgCnt;
    gfx_setupBg(0, textBg);
    gfx_setBgOffset(0, charbase);
    gfx_loadPalette(UiPalette, 0);
    font_setBgTarget(textBg);
    font_setStyle(FONT_STYLE_8PX_B, 9, 0);
    charbase = font_drawLines(0x101, 0x78, 0x20, 0xf0, text_getString(0), 1);
    font_drawLines(charbase, 0x78, 0x88, 0xf0, text_getString(4), 1);
    gfx_fadeInFromWhite(0x3f);
    sfx_requestSong(0);
    system_unknown08047e5c(0, 0x3c);
    system_unknown08047e5c(1, 0x5a);
}
