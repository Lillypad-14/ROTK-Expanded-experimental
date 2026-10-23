#include "sprite.h"
#include "game.h"
#include "variables.h"

/**
 * Tick and render the active actor list, keeping co-op gameplay running under overlays.
 *
 * @return The active list's update result.
 *
 * @romaddress 0x080016f8
 */
u8 actor_tickAll(void)
{
    u8 result;

    if (g_GameFlags.p & GAME_FLAG_OVERLAY)
    {
        g_SpriteFrameSubmitted = 1;
        result = actor_tickAllInList(&g_ActorListOverlay, TRUE);
        if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE)
        {
            g_SpriteFrameSubmitted = 0;
            actor_tickAllInList(&g_ActorListMain, FALSE);
        }
    }
    else
    {
        g_SpriteFrameSubmitted = 1;
        result = actor_tickAllInList(&g_ActorListMain, TRUE);
    }
    return result;
}
