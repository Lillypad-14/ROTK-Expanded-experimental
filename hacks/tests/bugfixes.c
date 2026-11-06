#include "patches/bugfixes/bugfixes.c"
#include <assert.h>
#include <stdio.h>

enum
{
    BASE_PLAIN = 0,
    BASE_SWORD = 1,
    BASE_AXE = 2,
    BASE_BOW = 3,
    BASE_STAFF = 4,
};

PlayerGlobals g_PlayerGlobals;
GameFlags g_GameFlags;

s32 math_percentOf(s32 base, u16 percent)
{
    return base * percent / 100;
}

s32 rng_range(s32 lo, s32 hi)
{
    (void)lo;
    (void)hi;
    return 1;
}

static s32 NextRoll = 1;
static u8 DeletedSlot;

s32 rng_rollRange(s32 lo, s32 hi)
{
    (void)lo;
    (void)hi;
    return NextRoll;
}

bool item_deleteFromInventory(u8 slot, u8 playerIndex)
{
    (void)playerIndex;
    DeletedSlot = slot;
    return TRUE;
}

// item stat hooks: record the slot each item's stats come off or go on for
static struct
{
    u32 item;
    u8 slot;
    bool remove;
} AffixCalls[8];
static u32 AffixCallCount;

void item_applyAffixStats(Item item, u8 inventorySlot, u8 playerIndex, bool remove)
{
    (void)playerIndex;
    AffixCalls[AffixCallCount].item = item.word;
    AffixCalls[AffixCallCount].slot = inventorySlot;
    AffixCalls[AffixCallCount].remove = remove;
    AffixCallCount++;
}

u32 item_getEquipSlot(u32 itemHandle, u8 playerIndex)
{
    (void)playerIndex;
    Item item = {.word = itemHandle};
    return item.d.baseIndex == BASE_STAFF ? INVENTORY_SLOT_MAINHAND : INVENTORY_SLOT_OFFHAND;
}

bool item_swapSlots(u8 slotA, u8 slotB, u8 playerIndex)
{
    Item a = PLAYER(playerIndex).inventory.array[slotA];
    PLAYER(playerIndex).inventory.array[slotA] = PLAYER(playerIndex).inventory.array[slotB];
    PLAYER(playerIndex).inventory.array[slotB] = a;
    return TRUE;
}

bool item_unequipToBackpack(u8 slot, u8 playerIndex)
{
    (void)slot;
    (void)playerIndex;
    return FALSE;
}

void player_updateEquipFlags(u8 playerIndex)
{
    (void)playerIndex;
}

u32 sfx_play(s32 sfxId)
{
    (void)sfxId;
    return 0;
}

s32 math_percentage(s32 value, s32 total)
{
    return total ? value * 100 / total : 0;
}

static const ItemMetadata Metadata[] = {
    [BASE_PLAIN] = {.flags = {.p = 0}},
    [BASE_SWORD] = {.flags = {.p = ITEM_FAMILY_SWORD}},
    [BASE_AXE] = {.flags = {.p = ITEM_FAMILY_AXE}},
    [BASE_BOW] = {.flags = {.p = ITEM_FAMILY_BOW}},
    [BASE_STAFF] = {.flags = {.p = ITEM_FAMILY_STAFF | ITEM_FLAG_TWO_HANDED}, .equipSfx = 0x128},
};

static const ItemStatRecord Bases[] = {
    [BASE_PLAIN] = {.stat0 = STAT_NONE},
    [BASE_SWORD] = {.stat0 = STAT_NONE},
    [BASE_AXE] = {.stat0 = STAT_NONE},
    [BASE_BOW] = {.stat0 = STAT_NONE},
    // a Crystal Crook of Jas Mynn: Fragile in the base's third stat
    [BASE_STAFF] = {.stat0 = STAT_NONE, .stat1 = STAT_NONE, .stat2 = STAT_FRAGILE, .val2 = 1},
};

enum
{
    PREFIX_RUSTED = 0,   // Fragile in stat1, the only slot the stock code read
    PREFIX_OBSIDIAN = 1, // Fragile in stat2
    PREFIX_CHEAP = 2,    // Fragile +0
    PREFIX_STURDY = 3,
};

static const ItemStatRecord Prefixes[] = {
    [PREFIX_RUSTED] = {.stat0 = STAT_NONE, .stat1 = STAT_FRAGILE, .val1 = 1},
    [PREFIX_OBSIDIAN] = {.stat0 = STAT_NONE, .stat1 = STAT_NONE, .stat2 = STAT_FRAGILE, .val2 = 1},
    [PREFIX_CHEAP] = {.stat0 = STAT_FRAGILE, .val0 = 0, .stat1 = STAT_NONE},
    [PREFIX_STURDY] = {.stat0 = STAT_NONE, .stat1 = STAT_NONE},
};

const ItemTypeInfo ItemTypeInfos[15] = {
    [ITEM_TYPE_SWORD_1H] = {.baseItems = Bases, .prefixes = Prefixes, .baseMetadata = Metadata},
    [ITEM_TYPE_AXE] = {.baseMetadata = Metadata},
    [ITEM_TYPE_BOW] = {.baseMetadata = Metadata},
    [ITEM_TYPE_STAFF] = {.baseMetadata = Metadata},
    [ITEM_TYPE_SHIELD] = {.baseMetadata = Metadata},
    [ITEM_TYPE_UNIQUE] = {.baseItems = Bases, .baseMetadata = Metadata},
};

static Item make(ItemType type, u8 base)
{
    Item item = EMPTY_ITEM;
    item.d.itemType = type;
    item.d.baseIndex = base;
    return item;
}

static s32 bonusWith(Item mainHand, Item offHand)
{
    memset(&g_PlayerGlobals, 0, sizeof g_PlayerGlobals);
    PLAYER(0).inventory.slots.mainhand = mainHand;
    PLAYER(0).inventory.slots.offhand = offHand;
    PLAYER_STAT(0, STAT_DAMAGE_WITH_BLADE) = 3;
    PLAYER_STAT(0, STAT_DAMAGE_WITH_AXE) = 5;
    PLAYER_STAT(0, STAT_DAMAGE_WITH_BOW) = 7;
    PLAYER_STAT(0, STAT_DAMAGE_WITH_SHIELD) = 11;
    PLAYER_STAT(0, STAT_MAX_HP) = 100;
    PLAYER(0).currentHp = 100;
    return bugfixes_getFlatDamageBonus(0);
}

// known-bugs #2: unique weapons get their family's bonus, like normal ones
static void testUniqueWeaponFamilyBonus(void)
{
    const Item empty = EMPTY_ITEM;
    assert(bonusWith(make(ITEM_TYPE_SWORD_1H, BASE_SWORD), empty) == 3);
    assert(bonusWith(make(ITEM_TYPE_UNIQUE, BASE_SWORD), empty) == 3);
    assert(bonusWith(make(ITEM_TYPE_UNIQUE, BASE_AXE), empty) == 5);
    assert(bonusWith(make(ITEM_TYPE_UNIQUE, BASE_BOW), empty) == 7);
    assert(bonusWith(make(ITEM_TYPE_UNIQUE, BASE_PLAIN), empty) == 0);
    // dual wield: each hand counts
    assert(bonusWith(make(ITEM_TYPE_UNIQUE, BASE_SWORD), make(ITEM_TYPE_AXE, BASE_AXE)) == 8);
    // a shield keeps its own bonus and adds no family bonus
    assert(bonusWith(make(ITEM_TYPE_UNIQUE, BASE_SWORD), make(ITEM_TYPE_SHIELD, BASE_PLAIN)) == 14);
    assert(bonusWith(empty, empty) == 0);
}

// known-bugs #3: a unique staff in the main hand sets the staff flag
static bool staffFlagWith(Item mainHand)
{
    memset(&g_PlayerGlobals, 0, sizeof g_PlayerGlobals);
    PLAYER(0).inventory.slots.mainhand = mainHand;
    PLAYER(0).inventory.slots.offhand = (Item)EMPTY_ITEM;
    bugfixes_updateEquipFlags(0);
    return (PLAYER(0).combatFlags.p & PLAYER_COMBAT_FLAG_MAIN_HAND_STAFF) != 0;
}

static void testUniqueStaffFlag(void)
{
    assert(staffFlagWith(make(ITEM_TYPE_STAFF, BASE_STAFF)));
    assert(staffFlagWith(make(ITEM_TYPE_UNIQUE, BASE_STAFF)));
    assert(!staffFlagWith(make(ITEM_TYPE_UNIQUE, BASE_SWORD)));
    assert(!staffFlagWith((Item)EMPTY_ITEM));
}

// known-bugs #5: corruption tiers are cumulative and follow the meter both ways
static u32 tiersAfter(u8 from, s8 delta)
{
    memset(&g_PlayerGlobals, 0, sizeof g_PlayerGlobals);
    g_GameFlags.p = 0;
    PLAYER(0).corruption = from;
    bugfixes_addCorruption(0, 0);
    bugfixes_addCorruption(0, delta);
    return g_GameFlags.p & BUGFIXES_CORRUPTION_TIERS;
}

static void testCorruptionTiers(void)
{
    const u32 t1 = GAME_FLAG_CORRUPTION_TIER_1, t2 = GAME_FLAG_CORRUPTION_TIER_2,
              t3 = GAME_FLAG_CORRUPTION_TIER_3;
    assert(tiersAfter(0, 95) == (t1 | t2 | t3));
    assert(tiersAfter(0, 85) == (t1 | t2));
    assert(tiersAfter(0, 60) == t1);
    assert(tiersAfter(95, -40) == t1);
    assert(tiersAfter(95, -10) == (t1 | t2));
    assert(tiersAfter(95, -100) == 0);
    assert(tiersAfter(10, 5) == 0);
}

// known-bugs #7: Fragile from any stat slot, and nothing breaks without a fragile hand
static Item withPrefix(Item item, u8 prefix)
{
    item.d.prefixIndex = prefix;
    return item;
}

static u8 brokenSlot(Item mainHand, Item offHand)
{
    memset(&g_PlayerGlobals, 0, sizeof g_PlayerGlobals);
    PLAYER(0).inventory.slots.mainhand = mainHand;
    PLAYER(0).inventory.slots.offhand = offHand;
    PLAYER(0).combatFlags.p = PLAYER_COMBAT_FLAG_MAIN_HAND_WEAPON |
                              (offHand.d.itemType != ITEM_TYPE_EMPTY ? PLAYER_COMBAT_FLAG_OFFHAND_WEAPON : 0);
    DeletedSlot = 0xff;
    bugfixes_fragileOnKill(0);
    return DeletedSlot;
}

static void testFragile(void)
{
    const Item sword = make(ITEM_TYPE_SWORD_1H, BASE_SWORD);
    const Item empty = EMPTY_ITEM;

    NextRoll = 1;
    assert(brokenSlot(withPrefix(sword, PREFIX_RUSTED), empty) == INVENTORY_SLOT_MAINHAND);
    assert(brokenSlot(withPrefix(sword, PREFIX_OBSIDIAN), empty) == INVENTORY_SLOT_MAINHAND);
    assert(brokenSlot(make(ITEM_TYPE_UNIQUE, BASE_STAFF), empty) == INVENTORY_SLOT_MAINHAND);
    assert(brokenSlot(sword, withPrefix(sword, PREFIX_OBSIDIAN)) == INVENTORY_SLOT_OFFHAND);
    // no fragile hand: the stock code deleted backpack slot 0 here
    assert(brokenSlot(withPrefix(sword, PREFIX_CHEAP), empty) == 0xff);
    assert(brokenSlot(withPrefix(sword, PREFIX_STURDY), empty) == 0xff);
    assert(brokenSlot(empty, empty) == 0xff);
    // a missed roll breaks nothing
    NextRoll = FRAGILE_PROC_PERCENT;
    assert(brokenSlot(withPrefix(sword, PREFIX_RUSTED), empty) == 0xff);
}

// known-bugs #9: a two-hander equipped over a lone offhand item takes that item's stats off for
// the offhand slot it was in, not the main hand it is swapped through
static void testEquipRemovalSlot(void)
{
    const Item twoHander = make(ITEM_TYPE_STAFF, BASE_STAFF);
    const Item dagger = make(ITEM_TYPE_SWORD_1H, BASE_SWORD);

    memset(&g_PlayerGlobals, 0, sizeof g_PlayerGlobals);
    g_PlayerGlobals.activePlayerIndex = 1;
    PLAYER(0).inventory.slots.mainhand = (Item)EMPTY_ITEM;
    PLAYER(0).inventory.slots.offhand = dagger;
    PLAYER(0).inventory.array[INVENTORY_SLOT_BACKPACK_0] = twoHander;
    AffixCallCount = 0;
    assert(bugfixes_equipFromBackpack(INVENTORY_SLOT_BACKPACK_0, 0));
    assert(AffixCallCount == 2);
    assert(AffixCalls[0].item == dagger.word && AffixCalls[0].remove);
    assert(AffixCalls[0].slot == INVENTORY_SLOT_OFFHAND);
    assert(AffixCalls[1].item == twoHander.word && !AffixCalls[1].remove);
    assert(AffixCalls[1].slot == INVENTORY_SLOT_MAINHAND);
    assert(PLAYER(0).inventory.slots.mainhand.word == twoHander.word);
    assert(PLAYER(0).inventory.array[INVENTORY_SLOT_BACKPACK_0].word == dagger.word);
}

int main(void)
{
    testUniqueWeaponFamilyBonus();
    testUniqueStaffFlag();
    testCorruptionTiers();
    testFragile();
    testEquipRemovalSlot();
    puts("bugfixes tests passed");
    return 0;
}
