#pragma once

#include "item.h"

#define GAUNTLET_SHOP_SIZE 4

typedef struct GauntletShopOffer
{
    Item item;
    u16 price;
    bool sold;
} GauntletShopOffer;

enum GauntletShopResult
{
    SHOP_BOUGHT = 0,
    SHOP_SOLD_OUT = 1,
    SHOP_NEED_GEMS = 2,
    SHOP_PACK_FULL = 3,
    SHOP_SOLD = 4,
    SHOP_CANT_SELL = 5,
    SHOP_GEMS_FULL = 6,
};

void gauntletShop_reset(void);
void gauntletShop_open(void);
const GauntletShopOffer *gauntletShop_offer(u32 slot);
u32 gauntletShop_buy(u32 slot);
u32 gauntletShop_salePrice(Item item);
bool gauntletShop_prepareSale(u32 inventorySlot);
Item gauntletShop_saleItem(void);
u32 gauntletShop_sell(void);
