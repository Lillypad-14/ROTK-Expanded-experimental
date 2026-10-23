#include "item.h"
#include "player.h"
#include "variables.h"

/**
 * Reset a player's inventory (16 slots) to empty.
 *
 * @romaddress 0x0802e8f0
 */
void player_clearInventory(u32 playerIndex)
{
    u8 index = playerIndex;
    u8 i = 0;
    Item *items = PLAYER(index).inventory.array;

    for (; i <= 15; i++)
    {
        item_clearSlot(&items[i].word);
    }
}
