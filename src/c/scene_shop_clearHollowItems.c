#include "item.h"
#include "player.h"
#include "scene/shop.h"
#include "variables.h"

/**
 * Empty the player's Ranger Hollow stash.
 *
 * @romaddress 0x0801fd70
 */
void scene_shop_clearHollowItems(u32 playerIndex)
{
    u8 index = playerIndex;
    u8 i = 0;
    Item *items = PLAYER(index).rangerHollowItems;

    for (; i <= 8; i++)
    {
        item_clearSlot(&items[i].word);
    }
}
