#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 a = (s16)baseStats.health;
 s32 b = baseStats.courage;
 a = (s16)(a << 2);
 a += b;
 a = (s16)a;
 a += baseStats.strength;
 return (s16)a;
}
