#define scene_shop_drawHollowSlotItem hdr_dummy
#include "gfx.h"
#include "item.h"
#include "scene/shop.h"
#undef scene_shop_drawHollowSlotItem
#include "variables.h"

#define ShopHollowSlotFrameAsset ((void *)0x08b89bd0) // new name needed

/**
 * Draw an item's icon and frame in one Ranger Hollow slot.
 *
 * @romaddress 0x0801f680
 */
void scene_shop_drawHollowSlotItem(Item item, u32 hollowSlot)
{
    u8 pos = hollowSlot + 1;

    gfx_loadBgAsset(2, ITEM_TYPE_INFO(item).baseGfx[item.d.baseIndex].hollowIcon, hollowSlot * 12 + 700,
                    hollowSlot + 7, ShopCursorPosHollow[pos].x / 8, ShopCursorPosHollow[pos].y / 8);
    gfx_loadBgAsset(3, ShopHollowSlotFrameAsset, hollowSlot * 12 + 0x328, 0, ShopCursorPosHollow[pos].x / 8,
                    ShopCursorPosHollow[pos].y / 8);
}
