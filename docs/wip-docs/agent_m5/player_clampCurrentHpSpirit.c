#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Re-clamp currentHp to STAT_MAX_HP (minimum 1) and currentSpirit to STAT_MAX_SPIRIT (minimum 0).
 *
 * @romaddress 0x080260bc
 */
void player_clampCurrentHpSpirit(u8 playerIndex)
{
    Player *base = g_PlayerGlobals.players;
    s16 *current = &(base + playerIndex)->currentHp;
    s16 *max = &(base + playerIndex)->stats[STAT_MAX_HP];

    if (*current > *max)
    {
        if (*max < 0)
        {
            goto floorHp;
        }
        *current = *max;
    }
    else if (*max <= 0)
    {
    floorHp:
        (base + playerIndex)->currentHp = 1;
    }

    current = &(base + playerIndex)->currentSpirit;
    max = &(base + playerIndex)->stats[STAT_MAX_SPIRIT];

    if (*current > *max)
    {
        if (*max >= 0)
        {
            *current = *max;
            return;
        }
    }
    else if (*max >= 0)
    {
        return;
    }
    (base + playerIndex)->currentSpirit = 0;
}
