#include "item.h"
#include "player.h"
#include "variables.h"

/**
 * Nonzero if the player meets the item's class-restriction bits and minimum level.
 *
 * @romaddress 0x0802ea88
 */
bool32 item_isUsableByPlayer(u32 itemHandle, u8 playerIndex)
{
    Item item;
    u32 heroId;
    u32 usable;
    u32 classBits;

    item.word = itemHandle;
    heroId = PLAYER(playerIndex).heroId;
    if (heroId == HERO_ID_SMEAGOL) {
        return FALSE;
    }
    if (heroId == HERO_ID_SAM) {
        heroId = HERO_ID_FRODO;
    }
    classBits = ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_CLASS_ALL;
    usable = ITEM_FLAG_CLASS_BIT(heroId) & classBits;
    if ((u8)item_getMinLevel(itemHandle) > PLAYER(playerIndex).level + 1) {
        usable = 0;
    }
    return usable;
}
