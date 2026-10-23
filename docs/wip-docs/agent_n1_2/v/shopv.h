#define scene_shop_clearHollowSlotItem hdr_dummy
#include "scene/shop.h"
#include "variables.h"
#undef scene_shop_clearHollowSlotItem
void gfx_clearBgRect(s32 layer, s32 col, s32 row, s32 width, s32 height);
__asm__(".global gfx_clearBgRect\n.set gfx_clearBgRect, 0x08012649");
