#include "player.h"
#include "variables.h"

void player_recomputeBaseStats(u8 playerIndex)
{
    Player *player = &PLAYER(playerIndex);

    player->baseStats.maxHp = player_computeMaxHp(playerIndex, player->baseStats);
    player_applyBaseStats(playerIndex);
}
