#include "item.h"
#include "player.h"
#include "scene/shop.h"
#include "variables.h"

/**
 * Spawn missing backpack item sprites and, for the Ranger Hollow, redraw hollow items.
 *
 * @romaddress 0x0801ebc8
 */
void scene_shop_refreshSlotSprites(bool backpackOnly)
{
    u8 i;
    u8 playerIndex;

    playerIndex = ACTIVE_PLAYER_INDEX;
    for (i = 0; i < BACKPACK_SLOT_COUNT; i++)
    {
        if (ITEM_TYPE_VIA_SHIFT(PLAYER(playerIndex).inventory.array[i + 8]) != ITEM_TYPE_EMPTY &&
            g_ShopBackpackSlotSprites[i] == NULL)
        {
            scene_shop_spawnBackpackSlotSprite(i);
            if (g_ShopCursor->actionState == i)
            {
                scene_shop_drawInfoPanel(g_ShopCursor->actionState, playerIndex);
            }
        }
    }
    if (!backpackOnly && g_ShopType == SHOP_TYPE_RANGER_HOLLOW)
    {
        for (i = 0; i < 9; i++)
        {
            if (ITEM_TYPE_VIA_SHIFT(PLAYER(playerIndex).rangerHollowItems[i]) != ITEM_TYPE_EMPTY)
            {
                scene_shop_drawHollowSlotItem(PLAYER(playerIndex).rangerHollowItems[i], i);
            }
        }
    }
}
