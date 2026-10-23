#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Max HP bonus from non-primary sources: the MAX_HP stat minus what the stat-boosted primaries
 * alone would give.
 *
 * @romaddress 0x0802dfd4
 */
s16 player_getMaxHpBonus(u8 playerIndex)
{
    u16 maxHpStat = PLAYER(playerIndex).stats[STAT_MAX_HP];
    HeroBaseStats statBase;

    statBase.strength = PLAYER(playerIndex).stats[STAT_STRENGTH];
    statBase.health = PLAYER(playerIndex).stats[STAT_HEALTH];
    statBase.courage = PLAYER(playerIndex).stats[STAT_COURAGE];
    statBase.maxHp = player_computeMaxHp(playerIndex, statBase);
    return maxHpStat - statBase.maxHp;
}
