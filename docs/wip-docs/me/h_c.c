#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 h = (s16)baseStats.health;
 u32 c = baseStats.courage;
 s32 r = (h << 2) + c;
 r = (s16)(r << 0);
 return (s16)(r + baseStats.strength);
}
