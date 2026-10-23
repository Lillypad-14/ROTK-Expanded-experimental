#include "item.h"
#include "player.h"

/**
 * Apply or remove one base item's or affix's stat modifiers.
 *
 * @param table a base-item, prefix, or suffix table
 * @param sign 1 to apply, -1 to remove
 *
 * @romaddress 0x0802dd10
 */
void item_affix_applyToPlayer(const ItemStatRecord *table, u8 inventorySlot, u8 playerIndex, u8 index, s8 sign)
{
    const ItemStatRecord *record = &table[index];

    if (record->stat0 != 0x5a)
    {
        player_addStat(inventorySlot, record->stat0, playerIndex, sign * (u8)record->val0);
    }
    if (record->stat1 != 0x5a)
    {
        player_addStat(inventorySlot, record->stat1, playerIndex, sign * (u8)record->val1);
    }
    if (record->stat2 != 0x5a)
    {
        player_addStat(inventorySlot, record->stat2, playerIndex, sign * (u8)record->val2);
    }
    if (record->stat3 != 0x5a)
    {
        player_addStat(inventorySlot, record->stat3, playerIndex, sign * (u8)record->val3);
    }
}
