#include "player.h"
#include "variables.h"

/** @romaddress 0x0801ff40 */
u32 scene_shop_attributePointCost(u32 playerIndex)
{
    u32 step = 100;
    u32 n = PLAYER(playerIndex).attributePointsPurchased;

    return n * step + 100;
}
