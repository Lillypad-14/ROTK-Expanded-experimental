#include "item.h"

/**
 * Set an item slot to the empty item.
 *
 * @romaddress 0x0802ea38
 */
void item_clearSlot(u32 *slot)
{
    Item *item = (Item *)slot;

    item->d.itemType = ITEM_TYPE_EMPTY;
    item->d.baseIndex = 0xff;
    item->d.prefixIndex = -1;
    item->d.suffixIndex = -1;
    item->d.runeIndex = RUNE_NONE;
}
