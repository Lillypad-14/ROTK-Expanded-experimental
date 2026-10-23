#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Re-clamp currentHp to STAT_MAX_HP and currentSpirit to STAT_MAX_SPIRIT after a stat change. A
 * non-positive max floors HP to 1 and a negative max floors spirit to 0.
 *
 * @romaddress 0x080260bc
 */
void player_clampCurrentHpSpirit(u8 playerIndex)
{
    if (PLAYER(playerIndex).currentHp > PLAYER_STAT(playerIndex, STAT_MAX_HP) &&
        PLAYER_STAT(playerIndex, STAT_MAX_HP) >= 0)
    {
        PLAYER(playerIndex).currentHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    }
    else if (PLAYER_STAT(playerIndex, STAT_MAX_HP) <= 0)
    {
        PLAYER(playerIndex).currentHp = 1;
    }
    if (PLAYER(playerIndex).currentSpirit > PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) &&
        PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) >= 0)
    {
        PLAYER(playerIndex).currentSpirit = PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT);
    }
    else if (PLAYER_STAT(playerIndex, STAT_MAX_SPIRIT) < 0)
    {
        PLAYER(playerIndex).currentSpirit = 0;
    }
}
