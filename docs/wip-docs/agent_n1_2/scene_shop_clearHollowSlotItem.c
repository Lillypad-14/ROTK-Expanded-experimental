#define scene_shop_clearHollowSlotItem hdr_dummy
#include "scene/shop.h"
#include "variables.h"
#undef scene_shop_clearHollowSlotItem
// header proto takes u8; u32 matches (add before narrowing)
void scene_shop_clearHollowSlotItem(u32 hollowSlot);

// Unnamed 0x08012648 (24 bytes): fills a BG rect with tile 0 (calls gfx_fillBgRect).
// Needs config name gfx_clearBgRect + gfx.h prototype (s32 layer, col, row, width, height).
void gfx_clearBgRect(s32 layer, s32 col, s32 row, s32 width, s32 height);
// Scratch-only stand-in so the harness resolves the call (not needed once named in config).
__asm__(".global gfx_clearBgRect\n.set gfx_clearBgRect, 0x08012649");

/**
 * Clear the item tiles of one Ranger Hollow cell on both item BG layers.
 *
 * @param hollowSlot Hollow cell (0..8)
 *
 * @romaddress 0x0801fe9c
 */
void scene_shop_clearHollowSlotItem(u32 hollowSlot)
{
    s32 col = ShopCursorPosHollow[(u8)(hollowSlot + 1)].x / 8;
    s32 row = ShopCursorPosHollow[(u8)(hollowSlot + 1)].y / 8;

    gfx_clearBgRect(2, col, row, 4, 4);
    gfx_clearBgRect(3, col, row, 4, 4);
}
