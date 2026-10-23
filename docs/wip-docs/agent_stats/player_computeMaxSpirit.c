#include "player.h"

/** @romaddress 0x08027374 */
s16 player_computeMaxSpirit(u8 playerIndex, HeroBaseStats baseStats)
{
    s32 courage = (s16)baseStats.courage;
    s32 health = baseStats.health;

    courage = (s16)(courage << 2);
    return (s16)(courage + health);
}
