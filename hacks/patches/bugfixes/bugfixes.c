// Fixes for bugs in the original game (docs/known-bugs.md). Each replaces a stock function through
// hookReplace in bugfixes.asm, ported from its src/c decompilation with the BUGFIX arm taken.

#include "combat.h"
#include "enemy.h"
#include "game.h"
#include "math.h"
#include "rng.h"
#include "sfx.h"
#include "item.h"
#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * combat_getFlatDamageBonus with weapon-family bonuses for unique weapons too (known-bugs #2): the
 * stock version tests the item type, which is ITEM_TYPE_UNIQUE for uniques, so Blademaster,
 * Axemaster and Rangemaster skipped them.
 */
s32 bugfixes_getFlatDamageBonus(s32 playerIndex)
{
    ItemType offHandType = PLAYER(playerIndex).inventory.slots.offhand.d.itemType;
    u16 damage = 0;

    damage += PLAYER_STAT(playerIndex, STAT_DAMAGE_FLAT);

    if (PLAYER(playerIndex).heroId == HERO_ID_ARAGORN &&
        (PLAYER(playerIndex).combatFlags.p & PLAYER_COMBAT_FLAG_MAIN_HAND_WEAPON) &&
        (PLAYER(playerIndex).combatFlags.p & PLAYER_COMBAT_FLAG_OFFHAND_WEAPON))
    {
        damage += PLAYER_STAT(playerIndex, STAT_ARAGORN_DUAL_WIELD_DAMAGE);
    }

    if (offHandType == ITEM_TYPE_SHIELD)
    {
        damage += PLAYER_STAT(playerIndex, STAT_DAMAGE_WITH_SHIELD);
    }

    for (u8 slot = INVENTORY_SLOT_MAINHAND; slot <= INVENTORY_SLOT_OFFHAND; slot++)
    {
        Item item = PLAYER(playerIndex).inventory.array[slot];
        if (item.d.itemType != ITEM_TYPE_EMPTY)
        {
            ItemFlags flags = ITEM_BASE_METADATA(item).flags;

            // knives, 1H swords and 2H swords are all familySword
            if (flags.d.familySword)
            {
                damage += PLAYER_STAT(playerIndex, STAT_DAMAGE_WITH_BLADE);
            }
            else if (flags.d.familyAxe)
            {
                damage += PLAYER_STAT(playerIndex, STAT_DAMAGE_WITH_AXE);
            }
            else if (flags.d.familyBow)
            {
                damage += PLAYER_STAT(playerIndex, STAT_DAMAGE_WITH_BOW);
            }
        }
    }

    if (PLAYER_STAT(playerIndex, STAT_LOW_HP_DAMAGE) != 0)
    {
        u32 hpPercent =
            math_percentage(PLAYER(playerIndex).currentHp, PLAYER_STAT(playerIndex, STAT_MAX_HP));
        if (hpPercent <= LOW_HP_BONUS_THRESHOLD_PERCENT)
        {
            damage += PLAYER_STAT(playerIndex, STAT_LOW_HP_DAMAGE);
        }
    }

    if (PLAYER_STAT(playerIndex, STAT_STRENGTH) > STRENGTH_DAMAGE_BASE)
    {
        s16 strength = PLAYER_STAT(playerIndex, STAT_STRENGTH);
        u8 strengthDamage = (strength - STRENGTH_DAMAGE_BASE) / STRENGTH_DAMAGE_STEP;

        damage += strengthDamage;
    }

    return damage;
}

#define BUGFIXES_MAINHAND (PLAYER(playerIndex).inventory.slots.mainhand)
#define BUGFIXES_OFFHAND (PLAYER(playerIndex).inventory.slots.offhand)
#define BUGFIXES_SET_COMBAT_FLAG_IF(flag, condition)                                               \
    {                                                                                              \
        if (condition)                                                                             \
        {                                                                                          \
            PLAYER(playerIndex).combatFlags.p |= (flag);                                           \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            PLAYER(playerIndex).combatFlags.p &= ~(flag);                                          \
        }                                                                                          \
    }

/**
 * player_updateEquipFlags with the main-hand staff flag set for unique staves (known-bugs #3): the
 * stock test asks for one unique id and a range that excludes it at once, so it never held.
 */
void bugfixes_updateEquipFlags(u8 playerIndex)
{
    BUGFIXES_SET_COMBAT_FLAG_IF(
        PLAYER_COMBAT_FLAG_MAIN_HAND_WEAPON,
        BUGFIXES_MAINHAND.d.itemType <= ITEM_TYPE_MACE || BUGFIXES_MAINHAND.d.itemType == ITEM_TYPE_BOW ||
            (BUGFIXES_MAINHAND.d.itemType == ITEM_TYPE_UNIQUE &&
             (BUGFIXES_MAINHAND.d.baseIndex <= UNIQUE_ID_SILVERAXE ||
              (BUGFIXES_MAINHAND.d.baseIndex >= UNIQUE_ID_STARFALL_BOW &&
               BUGFIXES_MAINHAND.d.baseIndex <= UNIQUE_ID_NIGHTFELL_BOW) ||
              (BUGFIXES_MAINHAND.d.baseIndex >= UNIQUE_ID_KING_ALDAZARS_LONGSWORD &&
               BUGFIXES_MAINHAND.d.baseIndex <= UNIQUE_ID_GREEN_MACE_OF_THE_OUTLANDER) ||
              (BUGFIXES_MAINHAND.d.baseIndex >= UNIQUE_ID_VELLA_OF_LORIENS_GOLDBOW &&
               BUGFIXES_MAINHAND.d.baseIndex <= UNIQUE_ID_ORC_LUMPS_BIG_BOW))));

    BUGFIXES_SET_COMBAT_FLAG_IF(
        PLAYER_COMBAT_FLAG_OFFHAND_WEAPON,
        BUGFIXES_OFFHAND.d.itemType <= ITEM_TYPE_SWORD_1H || BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_STAFF ||
            (BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_SWORD_2H && BUGFIXES_OFFHAND.d.baseIndex < 4) ||
            (BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_AXE && BUGFIXES_OFFHAND.d.baseIndex < 8) ||
            (BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_MACE && BUGFIXES_OFFHAND.d.baseIndex < 7) ||
            (BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_UNIQUE &&
             (BUGFIXES_OFFHAND.d.baseIndex <= UNIQUE_ID_STAFF_OF_FIVE_MAGES ||
              (BUGFIXES_OFFHAND.d.baseIndex >= UNIQUE_ID_KING_ALDAZARS_LONGSWORD &&
               BUGFIXES_OFFHAND.d.baseIndex <= UNIQUE_ID_OAKSTAFF_OF_OLD_THALCOS))));

    BUGFIXES_SET_COMBAT_FLAG_IF(PLAYER_COMBAT_FLAG_MAIN_HAND_STAFF,
                                BUGFIXES_MAINHAND.d.itemType != ITEM_TYPE_EMPTY &&
                                    (ITEM_BASE_METADATA(BUGFIXES_MAINHAND).flags.p & ITEM_FAMILY_STAFF));

    BUGFIXES_SET_COMBAT_FLAG_IF(
        PLAYER_COMBAT_FLAG_TWO_HANDED,
        (BUGFIXES_MAINHAND.d.itemType == ITEM_TYPE_EMPTY &&
         (PLAYER(playerIndex).combatFlags.p & PLAYER_COMBAT_FLAG_OFFHAND_WEAPON)) ||
            (BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_EMPTY &&
             (PLAYER(playerIndex).combatFlags.p & PLAYER_COMBAT_FLAG_MAIN_HAND_WEAPON)));

    BUGFIXES_SET_COMBAT_FLAG_IF(PLAYER_COMBAT_FLAG_SHIELD_EQUIPPED,
                                BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_SHIELD);

    if (BUGFIXES_OFFHAND.d.itemType == ITEM_TYPE_ARROW)
    {
        PLAYER(playerIndex).combatFlags.p |= PLAYER_COMBAT_FLAG_ARROWS_EQUIPPED;
        BUGFIXES_SET_COMBAT_FLAG_IF(PLAYER_COMBAT_FLAG_FIRE_ARROWS_EQUIPPED,
                                    BUGFIXES_OFFHAND.d.baseIndex == ARROW_INDEX_FIRE);
    }
    else
    {
        PLAYER(playerIndex).combatFlags.p &= ~PLAYER_COMBAT_FLAG_ARROWS_EQUIPPED;
        PLAYER(playerIndex).combatFlags.p &= ~PLAYER_COMBAT_FLAG_FIRE_ARROWS_EQUIPPED;
    }

    PLAYER(playerIndex).combatFlags.p |= PLAYER_COMBAT_FLAG_HAND_SPRITES_DIRTY;
}

#define BUGFIXES_CORRUPTION_MAX 100
#define BUGFIXES_CORRUPTION_TIER_1_MIN 51
#define BUGFIXES_CORRUPTION_TIER_2_MIN 81
#define BUGFIXES_CORRUPTION_TIER_3_MIN 91
// a single gain above this plays SFX_CORRUPTION_GAIN
#define BUGFIXES_CORRUPTION_GAIN_SFX_MIN 5
#define BUGFIXES_CORRUPTION_TIERS                                                                      (GAME_FLAG_CORRUPTION_TIER_1 | GAME_FLAG_CORRUPTION_TIER_2 | GAME_FLAG_CORRUPTION_TIER_3)

/**
 * player_addCorruption with the tier flags set from the meter as a whole (known-bugs #5): the stock
 * version only sets the new tier's own flag, so a big gain skips the lower tiers and a big drop can
 * leave tier 3 on. Each tier now sets every flag up to it and clears the ones above.
 */
void bugfixes_addCorruption(u8 playerIndex, s8 delta)
{
    u16 amount;
    u32 corruption;
    u32 tiers;

    if (delta > 0)
    {
        s16 resisted;

        if (PLAYER_STAT(playerIndex, STAT_CORRUPTION_RESIST_PERCENT) < 0)
        {
            resisted = math_percentOf(
                delta, (u16)(-PLAYER_STAT(playerIndex, STAT_CORRUPTION_RESIST_PERCENT)));
            amount = resisted + delta;
        }
        else
        {
            resisted = math_percentOf(
                delta, (u16)PLAYER_STAT(playerIndex, STAT_CORRUPTION_RESIST_PERCENT));
            amount = delta - resisted;
        }
        if ((s16)amount > BUGFIXES_CORRUPTION_GAIN_SFX_MIN)
        {
            sfx_play(SFX_CORRUPTION_GAIN);
        }
    }
    else
    {
        amount = delta;
    }

    amount = PLAYER(playerIndex).corruption + amount;
    if ((s16)amount > BUGFIXES_CORRUPTION_MAX)
    {
        PLAYER(playerIndex).corruption = BUGFIXES_CORRUPTION_MAX;
    }
    else if ((s16)amount < 0)
    {
        PLAYER(playerIndex).corruption = 0;
    }
    else
    {
        PLAYER(playerIndex).corruption = amount;
    }

    corruption = PLAYER(playerIndex).corruption;
    if (corruption >= BUGFIXES_CORRUPTION_TIER_3_MIN)
    {
        if (rng_range(0, 5) == 0)
        {
            sfx_play(SFX_CORRUPTION_TIER_3);
        }
        tiers = BUGFIXES_CORRUPTION_TIERS;
    }
    else if (corruption >= BUGFIXES_CORRUPTION_TIER_2_MIN)
    {
        tiers = GAME_FLAG_CORRUPTION_TIER_1 | GAME_FLAG_CORRUPTION_TIER_2;
    }
    else if (corruption >= BUGFIXES_CORRUPTION_TIER_1_MIN)
    {
        tiers = GAME_FLAG_CORRUPTION_TIER_1;
    }
    else
    {
        tiers = 0;
    }
    g_GameFlags.p = (g_GameFlags.p & ~BUGFIXES_CORRUPTION_TIERS) | tiers;
}

static bool32 bugfixes_recordIsFragile(const ItemStatRecord *record)
{
    return (record->stat0 == STAT_FRAGILE && record->val0 > 0) ||
           (record->stat1 == STAT_FRAGILE && record->val1 > 0) ||
           (record->stat2 == STAT_FRAGILE && record->val2 > 0) ||
           (record->stat3 == STAT_FRAGILE && record->val3 > 0);
}

/** Whether an item carries Fragile (above 0) in its base, prefix or suffix stats. */
static bool32 bugfixes_itemIsFragile(Item item)
{
    const ItemTypeInfo *type;

    if (item.d.itemType == ITEM_TYPE_EMPTY)
    {
        return FALSE;
    }
    type = &ITEM_TYPE_INFO(item);
    if (type->baseItems != NULL && bugfixes_recordIsFragile(&type->baseItems[item.d.baseIndex]))
    {
        return TRUE;
    }
    if (type->prefixes != NULL && item.d.prefixIndex != ITEM_AFFIX_NONE &&
        bugfixes_recordIsFragile(&type->prefixes[item.d.prefixIndex]))
    {
        return TRUE;
    }
    return type->suffixes != NULL && item.d.suffixIndex != ITEM_AFFIX_NONE &&
           bugfixes_recordIsFragile(&type->suffixes[item.d.suffixIndex]);
}

/**
 * combat_fragileOnKill reading Fragile from every stat slot of an item (known-bugs #7). The stock
 * version only finds Fragile in a prefix's second stat, so with Fragile elsewhere (Obsidian, Crystal
 * Crook of Jas Mynn) the kill roll found no fragile hand and deleted backpack slot 0 instead. Now a
 * roll with no fragile hand breaks nothing.
 */
void bugfixes_fragileOnKill(s32 playerIndex)
{
    Item mainHand = PLAYER(playerIndex).inventory.slots.mainhand;
    Item offhand = PLAYER(playerIndex).inventory.slots.offhand;
    u8 slot;

    if (rng_rollRange(1, 100) >= FRAGILE_PROC_PERCENT)
    {
        return;
    }
    if ((PLAYER(playerIndex).combatFlags.p & PLAYER_COMBAT_FLAG_MAIN_HAND_WEAPON) &&
        bugfixes_itemIsFragile(mainHand))
    {
        slot = INVENTORY_SLOT_MAINHAND;
    }
    else if ((PLAYER(playerIndex).combatFlags.p & PLAYER_COMBAT_FLAG_OFFHAND_WEAPON) &&
             bugfixes_itemIsFragile(offhand))
    {
        slot = INVENTORY_SLOT_OFFHAND;
    }
    else
    {
        return;
    }
    item_deleteFromInventory(slot, playerIndex);
    sfx_play(SFX_COMBAT_ITEM_BREAK);
}

/**
 * item_equipFromBackpack subtracting a replaced item's stats from the slot it was in (known-bugs
 * #9). Equipping a two-hander over an offhand, or an offhand item over a two-hander, first swaps
 * the old item into the new slot; the stock version then subtracted its stats for that new slot,
 * which removes the wrong share of hand-scaled bonuses and leaves the difference behind for good.
 */
bool bugfixes_equipFromBackpack(u8 backpackSlot, u8 playerIndex)
{
    Item item = PLAYER(playerIndex).inventory.array[backpackSlot];
    u8 inventorySlot = item_getEquipSlot(item.word, playerIndex);
    u8 removalSlot = inventorySlot;

    if (!INVENTORY_SLOT_IS_EQUIPMENT(inventorySlot))
    {
        if (playerIndex == ACTIVE_PLAYER_INDEX)
        {
            sfx_play(SFX_MENU_ERROR);
        }
        return FALSE;
    }
    if (inventorySlot == INVENTORY_SLOT_MAINHAND)
    {
        if (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_TWO_HANDED)
        {
            if (PLAYER(playerIndex).inventory.slots.offhand.d.itemType != ITEM_TYPE_EMPTY &&
                PLAYER(playerIndex).inventory.slots.mainhand.d.itemType == ITEM_TYPE_EMPTY)
            {
                removalSlot = INVENTORY_SLOT_OFFHAND;
                item_swapSlots(INVENTORY_SLOT_MAINHAND, INVENTORY_SLOT_OFFHAND, playerIndex);
            }
            else if (PLAYER(playerIndex).inventory.slots.offhand.d.itemType != ITEM_TYPE_EMPTY &&
                     !item_unequipToBackpack(INVENTORY_SLOT_OFFHAND, playerIndex))
            {
                if (playerIndex == ACTIVE_PLAYER_INDEX)
                {
                    sfx_play(SFX_MENU_ERROR);
                }
                return FALSE;
            }
        }
    }
    else if (inventorySlot == INVENTORY_SLOT_OFFHAND)
    {
        item = PLAYER(playerIndex).inventory.slots.mainhand;
        if (ITEM_TYPE(item) != ITEM_TYPE_EMPTY &&
            (ITEM_BASE_METADATA(item).flags.p & ITEM_FLAG_TWO_HANDED))
        {
            removalSlot = INVENTORY_SLOT_MAINHAND;
            item_swapSlots(INVENTORY_SLOT_OFFHAND, INVENTORY_SLOT_MAINHAND, playerIndex);
        }
    }

    item = PLAYER(playerIndex).inventory.array[inventorySlot];
    if (ITEM_TYPE(item) != ITEM_TYPE_EMPTY)
    {
        item_applyAffixStats(item, removalSlot, playerIndex, TRUE);
    }
    item_swapSlots(inventorySlot, backpackSlot, playerIndex);
    item = PLAYER(playerIndex).inventory.array[inventorySlot];
    item_applyAffixStats(item, inventorySlot, playerIndex, FALSE);
    player_updateEquipFlags(playerIndex);
    if (playerIndex == ACTIVE_PLAYER_INDEX && ITEM_BASE_METADATA(item).equipSfx < 0x128)
    {
        sfx_play(ITEM_BASE_METADATA(item).equipSfx);
    }
    return TRUE;
}

/**
 * combat_getEnemyFamilyDamageBonus with Woodsman (known-bugs #22): its stat was granted but never
 * read. Its text promises spiders, wargs and mumakil, which are this family.
 */
u16 bugfixes_getEnemyFamilyDamageBonus(s32 playerIndex, u8 enemyType)
{
    switch (EnemyTypeInfos[enemyType].family)
    {
    case ENEMY_FAMILY_ANIMALS:
        return PLAYER_STAT(playerIndex, STAT_DAMAGE_TO_ANIMALS);
    case ENEMY_FAMILY_CREBAIN:
        return PLAYER_STAT(playerIndex, STAT_DAMAGE_TO_CREBAIN);
    case ENEMY_FAMILY_WARGS:
        return PLAYER_STAT(playerIndex, STAT_DAMAGE_TO_WARGS) +
               PLAYER_STAT(playerIndex, STAT_WOODSMAN_DAMAGE);
    case ENEMY_FAMILY_ORCS:
        return PLAYER_STAT(playerIndex, STAT_DAMAGE_TO_ORCS);
    case ENEMY_FAMILY_URUKHAI:
        return PLAYER_STAT(playerIndex, STAT_DAMAGE_TO_URUKHAI);
    case ENEMY_FAMILY_NAZGUL:
        return PLAYER_STAT(playerIndex, STAT_DAMAGE_TO_NAZGUL);
    default:
        return 0;
    }
}
