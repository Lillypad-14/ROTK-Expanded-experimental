#include "gfx.h"
#include "scene/victory.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Fade out and tear down the victory scene.
 *
 * @romaddress 0x08020ad0
 */
void scene_victory_exit(void)
{
    gfx_alphaBlendOut(2, 1, 6, 4);
    gfx_disableBg(1);
    gfx_mosaicOutToBlack(0x3f);
    sprite_flushList(&g_ActorListMain);
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}
