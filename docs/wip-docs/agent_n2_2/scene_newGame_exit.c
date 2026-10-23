#include "actor.h"
#include "gfx.h"
#include "scene/newGame.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Fade out, flush the actor sprites and stop the sound.
 *
 * @romaddress 0x08014878
 */
void scene_newGame_exit(void)
{
    gfx_fadeOutToBlack(0x3f);
    sprite_flushList(&g_ActorListMain);
    gfx_commitBgs();
    sfx_stopAllOnExit();
}
