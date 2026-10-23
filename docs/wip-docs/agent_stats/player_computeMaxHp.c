#include "player.h"

/** @romaddress 0x08027348 */
s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
    s32 a = (s16)baseStats.health;
    s32 b = (s16)(a << 2) + baseStats.courage;

    b = (s16)b;
    b += baseStats.strength;
    return (s16)b;
}
