#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 a = (s16)baseStats.health;
 s32 b = (s16)(a << 2) + baseStats.courage;
 return (s16)((s16)b + baseStats.strength);
}
