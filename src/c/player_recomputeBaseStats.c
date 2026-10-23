#include "player.h"
#include "variables.h"

/**
 * Refresh baseStats.maxHp from the primaries, then apply the base stats.
 *
 * @romaddress 0x08027314
 */
void player_recomputeBaseStats(u8 playerIndex)
{
    Player *base = g_PlayerGlobals.players;
    Player *p = base + playerIndex;

    p->baseStats.maxHp = player_computeMaxHp(playerIndex, p->baseStats);
    player_applyBaseStats(playerIndex);
}
