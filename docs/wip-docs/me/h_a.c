#include "player.h"

s16 player_computeMaxHp(u8 playerIndex, HeroBaseStats baseStats)
{
 s32 h = baseStats.health;
 s32 c = baseStats.courage;
 s32 s = baseStats.strength;
 h = (s16)h;
 h = (s16)(h << 2) + c;
 h = (s16)h + s;
 return (s16)h;
}
