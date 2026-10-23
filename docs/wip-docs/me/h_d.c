#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 a = (s16)baseStats.health;
 s32 b = (s16)(a * 4) + baseStats.courage;
 b = (s16)b;
 b += baseStats.strength;
 return (s16)b;
}
