#include "actor.h"
#include "item.h"
#include "player.h"
#include "scene/shop.h"
#include "sprite.h"
#include "variables.h"

/**
 * Move an item from a Ranger Hollow slot into the inventory.
 *
 * @romaddress 0x0801fda0
 */
void scene_shop_takeItemFromHollow(u8 playerIndex, u32 inventorySlot, Item item, u8 hollowSlot)
{
    PLAYER(playerIndex).inventory.array[inventorySlot] = item;
    if (PLAYER(playerIndex).inventory.array[inventorySlot].d.itemType == ITEM_TYPE_UNIQUE &&
        (u8)(PLAYER(playerIndex).inventory.array[inventorySlot].d.baseIndex - 10) <= 15)
    {
        item_applyAffixStats(item, inventorySlot, playerIndex, FALSE);
    }
    if (playerIndex == ACTIVE_PLAYER_INDEX)
    {
        scene_shop_spawnBackpackSlotSprite(inventorySlot - 8);
        scene_shop_clearHollowSlotItem(hollowSlot);
        item_clearSlot(&PLAYER(playerIndex).rangerHollowItems[hollowSlot].word);
        scene_shop_drawInfoPanel(g_ShopCursor->actionState, playerIndex);
        sprite_setAnimation(g_ShopCursor, &ShopCursorAnimations[2]);
    }
}
