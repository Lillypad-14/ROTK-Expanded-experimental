#include "patches/gauntlet/gauntlet_shop.h"

#include "loot.h"
#include "memory.h"
#include "patches/gauntlet/gauntlet.h"
#include "player.h"
#include "variables.h"

#define SHOP_MAX_TIER 10
#define SHOP_BASE_PRICE 12
#define SHOP_ROOM_PRICE 2
#define SHOP_TIER_PRICE 6

typedef struct GauntletShopState
{
    GauntletShopOffer offers[GAUNTLET_SHOP_SIZE];
    u32 room;
    u8 hero;
    bool ready;
    Item sale;
    u8 saleSlot;
} GauntletShopState;

static GauntletShopState *Shop;

void gauntletShop_reset(void)
{
    if (Shop != NULL)
    {
        memory_free(Shop);
        Shop = NULL;
    }
}

static Item rollItem(u32 type, u32 tier)
{
    Item item = EMPTY_ITEM;
    Item chosen = EMPTY_ITEM;
    u32 count = 0;

    item.d.itemType = type;
    for (u32 base = 0; base < ITEM_TYPE_INFO(item).baseItemCount; base++)
    {
        item.d.baseIndex = base;
        if (ITEM_BASE(item).tier <= tier &&
            !(ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_ANCIENT) &&
            item_isUsableByPlayer(item.word, 0) &&
            item_getEquipSlot(item.word, 0) < INVENTORY_SLOT_BACKPACK_0 &&
            gauntlet_randBelow(++count) == 0)
        {
            chosen = item;
        }
    }
    if (chosen.d.itemType != ITEM_TYPE_EMPTY)
    {
        Item plain = chosen;
        u32 oldTier = g_LootTier;
        u32 hero = PLAYER(0).heroId;

        g_LootTier = tier;
        loot_rollPrefix(&chosen, 0xff, hero < 8 ? hero : HERO_ID_GANDALF, LOOT_UNIT_TYPE_CONTAINER);
        loot_rollSuffix(&chosen, 0);
        g_LootTier = oldTier;
        if (!item_isUsableByPlayer(chosen.word, 0) ||
            item_getEquipSlot(chosen.word, 0) >= INVENTORY_SLOT_BACKPACK_0)
        {
            chosen = plain;
        }
    }
    return chosen;
}

void gauntletShop_open(void)
{
    static const u8 Weapons[] = {ITEM_TYPE_KNIFE, ITEM_TYPE_BOW,      ITEM_TYPE_SWORD_1H,
                                 ITEM_TYPE_STAFF, ITEM_TYPE_SWORD_1H, ITEM_TYPE_AXE,
                                 ITEM_TYPE_KNIFE, ITEM_TYPE_KNIFE,    ITEM_TYPE_SWORD_1H,
                                 ITEM_TYPE_STAFF};
    static const u8 Armor[] = {ITEM_TYPE_ARMOR, ITEM_TYPE_CLOAK, ITEM_TYPE_GLOVES, ITEM_TYPE_SHOES};
    u32 room = gauntlet_roomsCleared() + 1;
    u32 hero = PLAYER(0).heroId;
    u32 tier = 1 + (room - 1) / 3;

    if (!gauntlet_isRunning())
    {
        return;
    }
    if (Shop == NULL)
    {
        Shop = memory_zalloc(sizeof(*Shop));
        if (Shop == NULL)
        {
            return;
        }
    }
    if (Shop->ready && Shop->room == room && Shop->hero == hero)
    {
        return;
    }
    Shop->room = room;
    Shop->hero = hero;
    Shop->ready = TRUE;
    Shop->sale = EMPTY_ITEM;
    Shop->saleSlot = 255;
    if (tier > SHOP_MAX_TIER)
    {
        tier = SHOP_MAX_TIER;
    }
    for (u32 slot = 0; slot < GAUNTLET_SHOP_SIZE; slot++)
    {
        u32 type = slot == 0   ? Weapons[hero < ARRAY_COUNT(Weapons) ? hero : 0]
                   : slot == 3 ? ITEM_TYPE_ACCESSORY
                               : Armor[gauntlet_randBelow(ARRAY_COUNT(Armor))];
        Item item = rollItem(type, tier);
        u32 price = SHOP_BASE_PRICE + room * SHOP_ROOM_PRICE;

        if (slot == 1 || slot == 2)
        {
            for (u32 attempt = 0;
                 attempt < ARRAY_COUNT(Armor) && item.d.itemType == ITEM_TYPE_EMPTY; attempt++)
            {
                item = rollItem(Armor[attempt], tier);
            }
        }
        if (item.d.itemType != ITEM_TYPE_EMPTY)
        {
            price += ITEM_BASE(item).tier * SHOP_TIER_PRICE;
            if (ITEM_HAS_PREFIX(item))
            {
                price += ITEM_PREFIX(item).tier * 3;
            }
            if (ITEM_HAS_SUFFIX(item))
            {
                price += ITEM_SUFFIX(item).tier * 4;
            }
        }
        Shop->offers[slot] = (GauntletShopOffer){item, price > 65535 ? 65535 : price, FALSE};
    }
}

const GauntletShopOffer *gauntletShop_offer(u32 slot)
{
    gauntletShop_open();
    return gauntlet_isRunning() && Shop != NULL && slot < GAUNTLET_SHOP_SIZE ? &Shop->offers[slot]
                                                                             : NULL;
}

u32 gauntletShop_buy(u32 slot)
{
    const GauntletShopOffer *offer = gauntletShop_offer(slot);

    if (offer == NULL || offer->sold || offer->item.d.itemType == ITEM_TYPE_EMPTY)
    {
        return SHOP_SOLD_OUT;
    }
    if (PLAYER(0).gems < offer->price)
    {
        return SHOP_NEED_GEMS;
    }
    for (u32 index = INVENTORY_SLOT_BACKPACK_0; index < ARRAY_COUNT(PLAYER(0).inventory.array);
         index++)
    {
        if (PLAYER(0).inventory.array[index].word == EMPTY_ITEM.word)
        {
            PLAYER(0).inventory.array[index] = offer->item;
            PLAYER(0).gems -= offer->price;
            Shop->offers[slot].sold = TRUE;
            return SHOP_BOUGHT;
        }
    }
    return SHOP_PACK_FULL;
}

u32 gauntletShop_salePrice(Item item)
{
    if (item.d.itemType == ITEM_TYPE_EMPTY)
    {
        return 0;
    }
    if (item.d.itemType == ITEM_TYPE_ARROW)
    {
        return 2 + item.d.prefixIndex / 5;
    }
    u32 price = 8 + ITEM_BASE(item).tier * 5;
    if (ITEM_HAS_PREFIX(item))
    {
        price += ITEM_PREFIX(item).tier * 2;
    }
    if (ITEM_HAS_SUFFIX(item))
    {
        price += ITEM_SUFFIX(item).tier * 3;
    }
    return price;
}

bool gauntletShop_prepareSale(u32 inventorySlot)
{
    gauntletShop_open();
    if (!gauntlet_isRunning() || Shop == NULL)
    {
        return FALSE;
    }
    Shop->sale = EMPTY_ITEM;
    Shop->saleSlot = 255;
    if (inventorySlot < INVENTORY_SLOT_BACKPACK_0 ||
        inventorySlot >= ARRAY_COUNT(PLAYER(0).inventory.array))
    {
        return FALSE;
    }
    Item item = PLAYER(0).inventory.array[inventorySlot];
    if (gauntletShop_salePrice(item) == 0)
    {
        return FALSE;
    }
    Shop->sale = item;
    Shop->saleSlot = inventorySlot;
    return TRUE;
}

Item gauntletShop_saleItem(void)
{
    return gauntlet_isRunning() && Shop != NULL ? Shop->sale : EMPTY_ITEM;
}

u32 gauntletShop_sell(void)
{
    if (!gauntlet_isRunning() || Shop == NULL || Shop->saleSlot < INVENTORY_SLOT_BACKPACK_0 ||
        Shop->saleSlot >= ARRAY_COUNT(PLAYER(0).inventory.array))
    {
        return SHOP_CANT_SELL;
    }
    Item item = PLAYER(0).inventory.array[Shop->saleSlot];
    u32 price = gauntletShop_salePrice(item);
    if (item.word != Shop->sale.word || price == 0)
    {
        return SHOP_CANT_SELL;
    }
    if (PLAYER(0).gems + price > 65535)
    {
        return SHOP_GEMS_FULL;
    }
    if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_CARRIED_PASSIVE)
    {
        item_applyAffixStats(item, Shop->saleSlot, 0, TRUE);
        if (item.d.baseIndex >= UNIQUE_WEAPON_BAND_END &&
            item.d.baseIndex < UNIQUE_CARRIED_PASSIVE_BAND_END)
        {
            PLAYER(0).uniquePassivesCollected &=
                ~(1u << (item.d.baseIndex - UNIQUE_WEAPON_BAND_END));
        }
    }
    PLAYER(0).inventory.array[Shop->saleSlot] = EMPTY_ITEM;
    PLAYER(0).gems += price;
    Shop->sale = EMPTY_ITEM;
    Shop->saleSlot = 255;
    return SHOP_SOLD;
}
