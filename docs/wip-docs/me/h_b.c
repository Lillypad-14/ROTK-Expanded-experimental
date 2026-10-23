#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s16 h = baseStats.health;
 u16 c = baseStats.courage;
 s32 r = ((s16)(h << 2) + c);
 r = (s16)r;
 return (s16)(r + baseStats.strength);
}
