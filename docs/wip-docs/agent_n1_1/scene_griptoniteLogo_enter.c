#define scene_griptoniteLogo_enter hdr_dummy
#include "gfx.h"
#include "scene/griptoniteLogo.h"
#include "sprite.h"
#include "variables.h"
#undef scene_griptoniteLogo_enter

// Needs config names: u32 at 0x08051160 and 0x08051164 (BG control values),
// assets at 0x08b7daf0 / 0x08b7e9cc (BG) and 0x08b7f8bc (palette).
#define LogoBgCnt0 (*(const u32 *)0x08051160)
#define LogoBgCnt1 (*(const u32 *)0x08051164)
#define LogoBg0Asset ((const void *)0x08b7daf0)
#define LogoBg1Asset ((const void *)0x08b7e9cc)
#define LogoPalette ((const void *)0x08b7f8bc)

/**
 * Set up the Griptonite logo screen and start its fade in.
 *
 * @romaddress 0x0801a898
 */
void scene_griptoniteLogo_enter(void)
{
    gfx_initDisplay(0);
    sprite_resetTileAllocatorForMode(0);
    gfx_setupBg(0, LogoBgCnt0);
    gfx_setBgOffset(0, 0);
    gfx_loadBgAsset(0, LogoBg0Asset, 1, 0, 0, 0);
    gfx_setBgControl(1, LogoBgCnt1);
    gfx_setBgOffset(1, 0);
    gfx_loadBgAsset(1, LogoBg1Asset, 1, 0, 0, 0);
    gfx_loadPalette(LogoPalette, 0);
    gfx_fadeSetDarken(3, 0x10);
    g_SceneCurrent.args[3] = 0;
    g_SceneCurrent.args[4] = 0x10;
}
