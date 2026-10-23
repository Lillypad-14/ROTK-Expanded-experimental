#include "item.h"
#include "player.h"
#include "variables.h"

u32 item_getEquipSlot(u32 itemHandle, u8 playerIndex)
{
    Item item;
    Item mainhand = PLAYER(playerIndex).inventory.slots.mainhand;
    u32 result = 8;
    Player *player = &PLAYER(playerIndex);
    u32 hero = player->heroId;
    u32 classFlags;

    item.word = itemHandle;
    if (hero == HERO_ID_SMEAGOL)
    {
        return result;
    }
    if (hero == HERO_ID_SAM)
    {
        hero = HERO_ID_FRODO;
    }
    classFlags = ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_CLASS_ALL;
    classFlags = ITEM_FLAG_CLASS_BIT(hero) & classFlags;
    if ((u8)item_getMinLevel(itemHandle) > player->level + 1)
    {
        classFlags = 0;
    }
    if (classFlags == 0)
    {
        return result;
    }
    if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_CARRIED_PASSIVE)
    {
        return result;
    }
    if ((ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_DUAL_WIELD_MASK) == ITEM_FLAG_DUAL_WIELD_MASK)
    {
        if (player->heroId == HERO_ID_ARAGORN && ITEM_TYPE(mainhand) != ITEM_TYPE_EMPTY &&
            !(ITEM_BASE_METADATA(mainhand).flags.p & ITEM_FLAG_TWO_HANDED) &&
            ITEM_TYPE(player->inventory.slots.offhand) == ITEM_TYPE_EMPTY)
        {
            return INVENTORY_SLOT_OFFHAND;
        }
        result = INVENTORY_SLOT_MAINHAND;
        if (PLAYER(playerIndex).heroId == HERO_ID_LEGOLAS)
        {
            result = INVENTORY_SLOT_OFFHAND;
        }
        return result;
    }
    if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_MAINHAND)
        result = INVENTORY_SLOT_MAINHAND;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_OFFHAND)
        result = INVENTORY_SLOT_OFFHAND;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_ARMOR)
        result = INVENTORY_SLOT_ARMOR;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_HELMET)
        result = INVENTORY_SLOT_HELMET;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_CLOAK)
        result = INVENTORY_SLOT_CLOAK;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_GLOVES)
        result = INVENTORY_SLOT_GLOVES;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_BOOTS)
        result = INVENTORY_SLOT_BOOTS;
    else if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_SLOT_JEWELRY)
    {
        if (PLAYER(playerIndex).heroId == HERO_ID_FRODO ||
            PLAYER(playerIndex).heroId == HERO_ID_SAM)
        {
            result = INVENTORY_SLOT_BOOTS;
            if (ITEM_TYPE(PLAYER(playerIndex).inventory.slots.jewelry) == ITEM_TYPE_EMPTY)
                result = INVENTORY_SLOT_JEWELRY;
        }
        else
            result = INVENTORY_SLOT_JEWELRY;
    }
    return result;
}
