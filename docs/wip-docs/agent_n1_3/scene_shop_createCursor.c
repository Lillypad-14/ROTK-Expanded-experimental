#include "actor.h"
#include "player.h"
#include "scene/shop.h"
#include "sprite.h"
#include "variables.h"

/**
 * Create the shop cursor actor on the first backpack cell and animate it for the item there.
 *
 * @romaddress 0x0801f728
 */
void scene_shop_createCursor(void)
{
    u8 animation;

    g_ShopCursor = actor_allocOverlay();
    sprite_setPosition(g_ShopCursor, ShopCursorPosBackpack[0].x << 16, ShopCursorPosBackpack[0].y << 16);
    g_ShopCursor->flags.p = ACTOR_FLAG_RENDER | ACTOR_FLAG_TICK_ALWAYS;
    g_ShopCursor->oam.objMode = 0;
    g_ShopCursor->oam.colors256 = 0;
    g_ShopCursor->oam.paletteNumber = 3;
    g_ShopCursor->oam.bgPriority = 0;
    g_ShopCursor->actionState = 0;
    g_ShopCursor->type = SHOP_CURSOR_SERVICE;
    animation = scene_shop_itemCursorAnimation(ACTIVE_PLAYER.inventory.slots.backpack_0, ACTIVE_PLAYER_INDEX);
    sprite_setAnimation(g_ShopCursor, &ShopCursorAnimations[animation]);
}
