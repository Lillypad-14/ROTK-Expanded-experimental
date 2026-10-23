#define scene_characterStats_exit hdr_dummy
#include "game.h"
#include "gfx.h"
#include "mission.h"
#include "multiplayer.h"
#include "player.h"
#include "scene.h"
#include "scene/characterStats.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"
#undef scene_characterStats_exit

// Needs name at 0x08027cdc (void, u8 playerIndex): runs the single-player stats exit.
void characterStats_leave(u8 playerIndex);

/**
 * Leave the character stats screen: fade out, tell the link partner in co-op, flush the
 * overlay actor list, and restore the game flags for the next scene.
 *
 * @romaddress 0x08027828
 */
void scene_characterStats_exit(void)
{
    MpMessageArgs msgArgs;

    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) && game_getNextSceneId() != SCENE_ID_GRIPTONITE_LOGO &&
        (s32)g_GameFlags.p >= 0)
    {
        gfx_fadeOutToBlackCoop(0x3f);
    }
    else
    {
        gfx_fadeOutToBlack(0x3f);
        if (game_getNextSceneId() == SCENE_ID_GRIPTONITE_LOGO)
        {
            sprite_flushList(&g_ActorListMain);
            mission_unknown0803915c();
            mission_unknown08039198();
        }
    }
    if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE)
    {
        // words[1]/[2] uninitialized: queued as-is
        msgArgs.words[0] = 0;
        multiplayer_queueMessage(0x1e, msgArgs);
    }
    else
    {
        characterStats_leave(g_PlayerGlobals.activePlayerIndex);
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
    gfx_commitBgs();
    gfx_clearPaletteAnims();
    sfx_stopAllOnExit();
}

void characterStats_leave(u8 playerIndex)
{
}
