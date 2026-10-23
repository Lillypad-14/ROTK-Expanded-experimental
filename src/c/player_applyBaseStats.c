#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Copy baseStats into the five primary stats and recompute STAT_MAX_HP / STAT_MAX_SPIRIT.
 *
 * @romaddress 0x08025f74
 */
void player_applyBaseStats(u8 playerIndex)
{
    Player *base = g_PlayerGlobals.players;
    Player *p = base + playerIndex;

    p->stats[STAT_COURAGE] = p->baseStats.courage;
    p->stats[STAT_DEFENSE] = p->baseStats.defense;
    p->stats[STAT_HEALTH] = p->baseStats.health;
    p->stats[STAT_ACCURACY] = p->baseStats.accuracy;
    p->stats[STAT_STRENGTH] = p->baseStats.strength;
    p->stats[STAT_MAX_HP] = player_computeMaxHp(playerIndex, p->baseStats);
    p->stats[STAT_MAX_SPIRIT] = player_computeMaxSpirit(playerIndex, p->baseStats);
}
