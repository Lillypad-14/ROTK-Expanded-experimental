#define scene_introLogo_enter hdr_dummy
#include "gfx.h"
#include "scene.h"
#include "scene/introLogo.h"
#include "sprite.h"
#include "variables.h"
#undef scene_introLogo_enter

/**
 * Set up the intro logo screen: two background layers showing the current logo frame.
 *
 * @romaddress 0x08039d50
 */
void scene_introLogo_enter(void)
{
    g_SceneCurrent.args[1] = 0;
    gfx_initDisplay(0);
    sprite_resetTileAllocatorForMode(0);
    gfx_setupBg(0, IntroLogoBg0Cnt);
    gfx_setBgOffset(0, 0);
    gfx_loadBgAsset(0, IntroLogoFrames[g_SceneCurrent.args[1]].bg0Asset, 1, 0, 0, 0);
    gfx_setBgControl(1, IntroLogoBg1Cnt);
    gfx_setBgOffset(1, 0);
    gfx_loadBgAsset(1, IntroLogoFrames[g_SceneCurrent.args[1]].bg1Asset, 1, 0, 0, 0);
    gfx_fadeSetDarken(3, 0x10);
    g_SceneCurrent.args[3] = 0;
    g_SceneCurrent.args[4] = 0x10;
}
