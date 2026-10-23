#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 a = (s16)baseStats.health;
 u32 b = baseStats.courage;
 s32 r = (s16)(a << 2) + b;
 return (s16)((s16)r + baseStats.strength);
}
