#include "game.h"
#include "gfx.h"
#include "scene/shop.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Fade out and tear down the shop scene.
 *
 * @romaddress 0x0801fc34
 */
void scene_shop_exit(void)
{
    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) && game_getNextSceneId() != SCENE_ID_GRIPTONITE_LOGO
        && (s32)g_GameFlags.p >= 0)
    {
        gfx_fadeOutToBlackCoop(0x3f);
    }
    else
    {
        gfx_fadeOutToBlack(0x3f);
    }
    sprite_flushList(&g_ActorListOverlay);
    g_GameFlags.p &= ~GAME_FLAG_REQUEST_OVERLAY;
    if (game_getNextSceneId() == SCENE_ID_GRIPTONITE_LOGO)
    {
        g_GameFlags.p &= ~GAME_FLAG_OVERLAY;
        g_GameFlags.p &= ~GAME_FLAG_WORLD_PARKED;
    }
    else
    {
        g_GameFlags.p |= GAME_FLAG_20;
    }
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}
