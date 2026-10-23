#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 b = baseStats.courage;
 s32 a = (s16)baseStats.health;
 a = (s16)(a << 2) + b;
 a = (s16)a;
 return (s16)(a + baseStats.strength);
}
