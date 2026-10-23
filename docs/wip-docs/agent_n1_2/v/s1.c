#include "shopv.h"
void scene_shop_clearHollowSlotItem(u32 hollowSlot)
{
    s32 x = ShopCursorPosHollow[(u8)(hollowSlot + 1)].x;
    s32 y = ShopCursorPosHollow[(u8)(hollowSlot + 1)].y;
    s32 col = x / 8;
    s32 row = y / 8;
    s32 size = 4;

    gfx_clearBgRect(2, col, row, size, size);
    gfx_clearBgRect(3, col, row, size, size);
}
