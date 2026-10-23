#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Add delta to a player's current spirit (clamped to [0, STAT_MAX_SPIRIT]) and mark the HUD spirit
 * display dirty.
 *
 * @romaddress 0x080261d8
 */
void player_addSpirit(u8 playerIndex, s16 delta)
{
    PLAYER(playerIndex).currentSpirit += delta;
    if (PLAYER(playerIndex).currentSpirit > PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT))
    {
        PLAYER(playerIndex).currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    }
    if (PLAYER(playerIndex).currentSpirit < 0)
    {
        PLAYER(playerIndex).currentSpirit = 0;
    }
    g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_SPIRIT_DIRTY;
}
