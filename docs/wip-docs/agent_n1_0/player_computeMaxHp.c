#define player_computeMaxHp hdr_dummy
#include "player.h"
#undef player_computeMaxHp

/**
 * Max HP from the base stats: 4 * health + courage + strength.
 *
 * @romaddress 0x08027348
 */
s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
    s32 health = (s16)baseStats.health;
    s32 sum;
    (void)playerIndex;
    sum = (s16)(health << 2) + baseStats.courage;
    sum = (s16)sum;
    return (s16)(sum + baseStats.strength);
}
