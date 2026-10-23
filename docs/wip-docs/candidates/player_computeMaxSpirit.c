#include "player.h"

/**
 * Max spirit from the base stats: 4 * courage + health.
 *
 * @romaddress 0x08027374
 */
s16 player_computeMaxSpirit(u8 playerIndex, HeroBaseStats baseStats)
{
    s32 courage = (s16)baseStats.courage << 2;

    return (s16)(courage + baseStats.health);
}
