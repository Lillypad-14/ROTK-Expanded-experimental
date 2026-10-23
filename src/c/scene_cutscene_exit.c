#include "gfx.h"
#include "player.h"
#include "scene.h"
#include "scene/cutscene.h"
#include "sfx.h"
#include "sprite.h"
#include "text.h"
#include "variables.h"

/**
 * Fade out and clear the cutscene unless the next scene continues the chain.
 *
 * @romaddress 0x08045e44
 */
void scene_cutscene_exit(void)
{
    if (g_SceneCurrent.args[0] - CUTSCENE_PAGE_FIRST > 1)
    {
        gfx_fadeOutToBlack(0x3f);
        sfx_stopAllOnExit();
        sprite_flushList(&g_ActorListMain);
        player_resetHuds();
        text_objClearSlots();
        gfx_commitBgs();
    }
}
