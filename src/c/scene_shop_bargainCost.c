#include "player.h"
#include "scene/shop.h"
#include "variables.h"

/**
 * Gem cost of a bargain: (30 + 3 per loot tier + player level) * 2.
 *
 * @romaddress 0x0801ff80
 */
u32 scene_shop_bargainCost(u32 playerIndex)
{
    u16 base = g_LootTier * 3 + 30 + PLAYER(playerIndex).level;

    return (u16)(base * 2);
}
