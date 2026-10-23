#include "player.h"
#include "variables.h"

#define ATTRIBUTE_POINT_BASE_COST 100
#define ATTRIBUTE_POINT_COST_STEP 100

/**
 * Gold cost of a player's next purchased attribute point.
 *
 * @romaddress 0x0801ff40
 */
u32 scene_shop_attributePointCost(u32 playerIndex)
{
    return PLAYER(playerIndex).attributePointsPurchased * ATTRIBUTE_POINT_COST_STEP +
           ATTRIBUTE_POINT_BASE_COST;
}
