#include "gfx.h"
#include "sfx.h"
#include "sprite.h"
#include "system.h"
#include "scene/unknown16.h"
#include "variables.h"

/**
 * @romaddress 0x080303cc
 */
void scene_unknown16_enter(void)
{
    sfx_requestSong(0);
    gfx_initDisplay(0);
    gfx_setupBg(1, MainMenuBgCnt1);
    gfx_loadBgAsset(1, Unknown16Bg1Asset, 1, 0, 0, 0);
    gfx_setupBg(3, MainMenuBgCnt3);
    gfx_loadBgAsset(3, Unknown16Bg3Asset, 0xfb, 0, 0, 0);
    gfx_setupBg(2, Unknown16Bg2Cnt);
    gfx_loadBgAsset(2, Unknown16Bg2Asset, 0x1f5, 0, 0, 0);
    gfx_setBgScrollSpeedX(1, -0x6666);
    gfx_setBgScrollSpeedX(3, 0x6666);
    sprite_endFrame();
    gfx_fadeInFromBlackWhileRunning(0x3f);
    system_unknown08047e5c(0, 0x3c);
    system_unknown08047e5c(1, 0x5a);
}
