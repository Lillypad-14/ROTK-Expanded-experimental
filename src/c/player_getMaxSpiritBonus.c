#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Max spirit bonus from non-primary sources: the MAX_SPIRIT stat minus what the stat-boosted
 * primaries alone would give.
 *
 * @romaddress 0x0802e028
 */
s16 player_getMaxSpiritBonus(u8 playerIndex)
{
    HeroBaseStats statBase;
    s16 maxSpiritStat;
    s16 spirit;

    statBase.strength = PLAYER(playerIndex).stats[STAT_STRENGTH];
    statBase.health = PLAYER(playerIndex).stats[STAT_HEALTH];
    statBase.courage = PLAYER(playerIndex).stats[STAT_COURAGE];
    statBase.maxHp = player_computeMaxHp(playerIndex, statBase);
    maxSpiritStat = PLAYER(playerIndex).stats[STAT_MAX_SPIRIT];
    spirit = player_computeMaxSpirit(playerIndex, statBase);
    return maxSpiritStat - spirit;
}
