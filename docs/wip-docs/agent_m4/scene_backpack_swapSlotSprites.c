#include "actor.h"
#include "scene/backpack.h"
#include "variables.h"

void scene_backpack_unknown0802d578(u8 slotA, u8 slotB);

#define ICON_SLOT_INDEX(icon) (*(u16 *)((u8 *)(icon) + 0xf0))

/**
 * Exchange two backpack slots' item icon sprites and redraw both cells.
 *
 * @romaddress 0x0802eb3c
 */
void scene_backpack_swapSlotSprites(u8 slotA, u8 slotB)
{
    Actor *tmp;

    if (g_BackpackSlotSprites[slotA] != NULL)
    {
        ICON_SLOT_INDEX(g_BackpackSlotSprites[slotA]) = slotB;
    }
    if (g_BackpackSlotSprites[slotB] != NULL)
    {
        ICON_SLOT_INDEX(g_BackpackSlotSprites[slotB]) = slotA;
    }
    scene_backpack_unknown0802d578(slotA, slotB);
    scene_backpack_unknown0802d578(slotB, slotA);
    tmp = g_BackpackSlotSprites[slotA];
    g_BackpackSlotSprites[slotA] = g_BackpackSlotSprites[slotB];
    g_BackpackSlotSprites[slotB] = tmp;
}
