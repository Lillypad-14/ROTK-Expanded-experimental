#define scene_backpack_unknown0802d578 hdr_dummy
#include "actor.h"
#include "player.h"
#include "scene/backpack.h"
#include "sprite.h"
#include "variables.h"
#undef scene_backpack_unknown0802d578

// Needs config: Vector2Int[8..15] table at 0x08062aa4 (backpack cell positions, indexed by
// slot) and names for 0x0802e6bc (void, Item, u8 slot) and 0x0802e774 (void, Item, u8 slot).
#define BackpackCellPositions ((const Vector2Int *)0x08062aa4)
void scene_backpack_unknown0802e6bc(Item item, u8 slot);
void scene_backpack_unknown0802e774(Item item, u8 slot);

/**
 * Update the icon sprite of slotA to show the item that moved into slotB.
 *
 * @romaddress 0x0802d578
 */
void scene_backpack_unknown0802d578(u8 slotA, u8 slotB)
{
    u8 playerIndex = ACTIVE_PLAYER_INDEX;
    Actor *sprite = g_BackpackSlotSprites[slotA];
    Item item;

    if (sprite != NULL)
    {
        item = PLAYER(playerIndex).inventory.array[slotB];
        if (slotB <= 7)
        {
            scene_backpack_unknown0802e6bc(item, slotB);
            g_BackpackSlotSprites[slotA]->flags.p &= ~ACTOR_FLAG_RENDER;
        }
        else
        {
            sprite_setPosition(sprite, BackpackCellPositions[slotB].x << 16, BackpackCellPositions[slotB].y << 16);
            scene_backpack_unknown0802e774(item, slotA);
        }
        g_BackpackSlotSprites[slotA]->flags.p |= ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
    }
    else if (slotB <= 7)
    {
        scene_backpack_clearSlotRect(slotB);
    }
    else
    {
        g_BackpackSlotSprites[slotB]->flags.p &= ~ACTOR_FLAG_RENDER;
    }
}

void scene_backpack_unknown0802e6bc(Item item, u8 slot)
{
}

void scene_backpack_unknown0802e774(Item item, u8 slot)
{
}
