#define scene_gcnLinkMenu_exit hdr_dummy
#include "gfx.h"
#include "scene/gcnLinkMenu.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"
#undef scene_gcnLinkMenu_exit

/**
 * Fade out and tear down the GameCube link menu.
 *
 * @romaddress 0x08021bd8
 */
void scene_gcnLinkMenu_exit(void)
{
    gfx_fadeOutToBlack(0x3f);
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
    sprite_flushList(&g_ActorListMain);
}
