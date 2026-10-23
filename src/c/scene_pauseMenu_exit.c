#include "gfx.h"
#include "scene.h"
#include "scene/pauseMenu.h"
#include "scene/gameplay.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Leave the pause menu: blend and mosaic out, flush sprites, and drop the parked
 * gameplay world unless the next scene is the gameplay resume.
 *
 * @romaddress 0x08039d04
 */
void scene_pauseMenu_exit(void)
{
    gfx_alphaBlendOut(2, 1, 6, 4);
    gfx_disableBg(1);
    gfx_mosaicOutToBlack(GFX_BLEND_ALL_TARGETS);
    sprite_flushList(&g_ActorListMain);
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
    if ((g_SceneNext.id & ~0x80) == SCENE_ID_MAIN_MENU)
    {
        scene_gameplay_discardParkedWorld();
    }
}
