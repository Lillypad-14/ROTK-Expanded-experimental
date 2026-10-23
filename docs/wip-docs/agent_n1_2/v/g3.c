#include "game.h"
#include "gfx.h"
#include "mission.h"
#include "scene.h"
#include "scene/skillTree.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Leave the skill tree: fade out, flush the overlay actor list, and restore the game flags
 * for the next scene.
 *
 * @romaddress 0x08041800
 */
void scene_skillTree_exit(void)
{
    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) && game_getNextSceneId() != SCENE_ID_GRIPTONITE_LOGO &&
        (s32)g_GameFlags.p >= 0) {
        gfx_fadeOutToBlackCoop(0x3f);
    } else {
        gfx_fadeOutToBlack(0x3f);
        if (game_getNextSceneId() == SCENE_ID_GRIPTONITE_LOGO) {
            sprite_flushList(&g_ActorListMain);
            mission_unknown0803915c();
            mission_unknown08039198();
        }
    }
    sprite_flushList(&g_ActorListOverlay);
    g_GameFlags.p &= ~GAME_FLAG_18;
    if (game_getNextSceneId() == SCENE_ID_GRIPTONITE_LOGO) {
        g_GameFlags.p = (g_GameFlags.p & ~GAME_FLAG_OVERLAY) & -9;
    } else {
        g_GameFlags.p |= GAME_FLAG_20;
    }
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}
