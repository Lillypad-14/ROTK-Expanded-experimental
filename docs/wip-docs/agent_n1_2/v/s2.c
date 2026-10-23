#include "shopv.h"
void scene_shop_clearHollowSlotItem(u32 hollowSlot)
{
    u8 cell = hollowSlot + 1;
    s32 x = ShopCursorPosHollow[cell].x;
    s32 col = x / 8;
    s32 y = ShopCursorPosHollow[cell].y;
    s32 row = y / 8;
    s32 size = 4;

    gfx_clearBgRect(2, col, row, 4, size);
    gfx_clearBgRect(3, col, row, 4, size);
}
