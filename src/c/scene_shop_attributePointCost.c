#include "scene/shop.h"
#include "variables.h"

/**
 * Gem cost of the next attribute point: 100 plus 100 per point already bought.
 *
 * @romaddress 0x0801ff40
 */
u32 scene_shop_attributePointCost(u32 playerIndex)
{
    u32 step = 100;
    u32 n = PLAYER(playerIndex).attributePointsPurchased;

    return n * step + 100;
}
