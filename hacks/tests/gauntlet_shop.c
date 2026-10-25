#include "patches/gauntlet/gauntlet_shop.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

PlayerGlobals g_PlayerGlobals;
u8 g_LootTier;
static u32 cleared, rolls;
static bool running = TRUE, highAffix, failAllocation;
static u32 allocations;
static const ItemStatRecord Bases[] = {{.tier = 1}, {.tier = 3}};
static const ItemStatRecord Affixes[] = {{.tier = 2}};
static const ItemMetadata Metadata[2] = {
    {0},
    {.flags.p = ITEM_FLAG_CARRIED_PASSIVE},
};
#define TYPE                                                                                       \
    {.baseItems = Bases,                                                                           \
     .baseMetadata = Metadata,                                                                     \
     .baseItemCount = 2,                                                                           \
     .prefixes = Affixes,                                                                          \
     .suffixes = Affixes}
const ItemTypeInfo ItemTypeInfos[15] = {
    TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE,
};

void *memory_zalloc(size_t size)
{
    if (failAllocation)
    {
        return NULL;
    }
    allocations++;
    return calloc(1, size);
}
void memory_free(void *pointer)
{
    allocations--;
    free(pointer);
}

u32 gauntlet_roomsCleared(void)
{
    return cleared;
}
bool32 gauntlet_isRunning(void)
{
    return running;
}
s32 rng_mod(u16 n)
{
    rolls++;
    return n == 0 ? 0 : rolls % (n + 1);
}
bool32 item_isUsableByPlayer(u32 word, u8 player)
{
    Item item = {.word = word};
    return item.d.itemType != ITEM_TYPE_GLOVES && item.d.itemType != ITEM_TYPE_SHOES &&
           PLAYER(player).heroId != HERO_ID_SMEAGOL &&
           (!highAffix || item.d.prefixIndex == ITEM_AFFIX_NONE);
}
u32 item_getEquipSlot(u32 word, u8 player)
{
    Item item = {.word = word};
    (void)player;
    return item.d.itemType <= ITEM_TYPE_BOW ? INVENTORY_SLOT_MAINHAND : INVENTORY_SLOT_ARMOR;
}
void loot_rollPrefix(Item *item, s16 extra, u8 hero, u8 unit)
{
    (void)extra;
    (void)unit;
    assert(hero < 8 && g_LootTier >= 1 && g_LootTier <= 10);
    item->d.prefixIndex = 0;
}
void loot_rollSuffix(Item *item, s16 extra)
{
    (void)item;
    (void)extra;
}

static u32 passiveRemovals;
void item_applyAffixStats(Item item, u8 slot, u8 player, bool remove)
{
    (void)item;
    assert(slot >= 8 && player == 0 && remove);
    passiveRemovals++;
}

int main(void)
{
    PLAYER(0).heroId = HERO_ID_ARAGORN;
    g_LootTier = 7;
    for (u32 i = 0; i < 16; i++)
    {
        PLAYER(0).inventory.array[i] = EMPTY_ITEM;
    }
    gauntletShop_reset();
    GauntletShopOffer first = *gauntletShop_offer(0);
    assert(first.item.d.itemType == ITEM_TYPE_SWORD_1H && first.price > 0);
    assert(g_LootTier == 7);
    u32 count = rolls;
    gauntletShop_open();
    assert(rolls == count && gauntletShop_offer(0)->item.word == first.item.word);
    assert(gauntletShop_buy(0) == SHOP_NEED_GEMS);
    assert(!gauntletShop_offer(0)->sold);
    PLAYER(0).gems = 1000;
    for (u32 i = 8; i < 16; i++)
    {
        PLAYER(0).inventory.array[i] = first.item;
    }
    assert(gauntletShop_buy(0) == SHOP_PACK_FULL && PLAYER(0).gems == 1000);
    assert(!gauntletShop_offer(0)->sold);
    PLAYER(0).inventory.array[8] = EMPTY_ITEM;
    assert(gauntletShop_buy(0) == SHOP_BOUGHT);
    assert(PLAYER(0).gems == 1000 - first.price);
    assert(PLAYER(0).inventory.array[8].word == first.item.word);
    assert(PLAYER(0).inventory.array[INVENTORY_SLOT_MAINHAND].word == EMPTY_ITEM.word);
    assert(gauntletShop_buy(0) == SHOP_SOLD_OUT);
    assert(gauntletShop_buy(4) == SHOP_SOLD_OUT);
    gauntletShop_open();
    assert(gauntletShop_offer(0)->sold && rolls == count);
    u32 gems = PLAYER(0).gems;
    u32 salePrice = gauntletShop_salePrice(first.item);
    assert(salePrice > 0 && salePrice < first.price);
    assert(!gauntletShop_prepareSale(INVENTORY_SLOT_MAINHAND));
    assert(!gauntletShop_prepareSale(16));
    assert(gauntletShop_prepareSale(8));
    assert(gauntletShop_saleItem().word == first.item.word);
    assert(PLAYER(0).gems == gems);
    assert(gauntletShop_sell() == SHOP_SOLD);
    assert(PLAYER(0).gems == gems + salePrice);
    assert(PLAYER(0).inventory.array[8].word == EMPTY_ITEM.word);
    assert(gauntletShop_sell() == SHOP_CANT_SELL);
    assert(!gauntletShop_prepareSale(8));
    PLAYER(0).inventory.array[8] = first.item;
    assert(gauntletShop_prepareSale(8));
    PLAYER(0).inventory.array[8].d.prefixIndex = ITEM_AFFIX_NONE;
    gems = PLAYER(0).gems;
    assert(gauntletShop_sell() == SHOP_CANT_SELL && PLAYER(0).gems == gems);
    assert(gauntletShop_prepareSale(8));
    PLAYER(0).gems = 65535;
    assert(gauntletShop_sell() == SHOP_GEMS_FULL);
    assert(PLAYER(0).inventory.array[8].word != EMPTY_ITEM.word);
    PLAYER(0).gems = 65535 - gauntletShop_salePrice(PLAYER(0).inventory.array[8]);
    assert(gauntletShop_sell() == SHOP_SOLD && PLAYER(0).gems == 65535);
    assert(passiveRemovals == 0);
    Item passive = EMPTY_ITEM;
    passive.d.itemType = ITEM_TYPE_ARMOR;
    passive.d.baseIndex = 1;
    PLAYER(0).inventory.array[15] = passive;
    PLAYER(0).gems = 0;
    assert(gauntletShop_prepareSale(15));
    assert(gauntletShop_sell() == SHOP_SOLD && passiveRemovals == 1);
    assert(PLAYER(0).inventory.array[15].word == EMPTY_ITEM.word);
    Item arrows = EMPTY_ITEM;
    arrows.d.itemType = ITEM_TYPE_ARROW;
    arrows.d.prefixIndex = 100;
    assert(gauntletShop_salePrice(arrows) == 22);
    cleared++;
    assert(!gauntletShop_offer(0)->sold && rolls > count);
    count = rolls;
    gauntletShop_reset();
    gauntletShop_open();
    assert(rolls > count);
    highAffix = TRUE;
    for (u32 hero = 0; hero < 10; hero++)
    {
        PLAYER(0).heroId = hero;
        gauntletShop_reset();
        for (u32 slot = 0; slot < 4; slot++)
        {
            const GauntletShopOffer *offer = gauntletShop_offer(slot);
            if (hero == HERO_ID_SMEAGOL)
            {
                assert(offer->item.d.itemType == ITEM_TYPE_EMPTY);
            }
            else
            {
                assert(offer->item.d.itemType != ITEM_TYPE_EMPTY &&
                       offer->item.d.prefixIndex == ITEM_AFFIX_NONE);
            }
        }
        assert(g_LootTier == 7);
    }
    gauntletShop_reset();
    assert(allocations == 0);
    failAllocation = TRUE;
    assert(gauntletShop_offer(0) == NULL && gauntletShop_buy(0) == SHOP_SOLD_OUT);
    assert(allocations == 0);
    running = FALSE;
    assert(gauntletShop_offer(0) == NULL && gauntletShop_buy(0) == SHOP_SOLD_OUT);
    puts("PASS: shop stock persistence, refresh, compatibility, purchase accounting and capacity");
    return 0;
}
