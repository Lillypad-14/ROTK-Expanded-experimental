#if !HACK_ringwraith || !HACK_skill_expansion || !HACK_text_overrides
#error "legacy_items requires ringwraith, skill_expansion and text_overrides"
#endif

#include "patches/legacy_items/legacy_items.h"
#include "patches/mod_save/save_layout.h"
#include "patches/mod_save/save_sram.h"
#include "lib/pc_host.h"
#include "player.h"
#include "save.h"
#include "stats.h"
#include "text.h"

#define TWO_TOWERS_RECORD_SIZE 16
#define TWO_TOWERS_RECORD_STATS 5
#define TWO_TOWERS_STAT_NONE 0x40
#define TWO_TOWERS_STAT_DAMAGE 0
#define TWO_TOWERS_STAT_TWO_HANDED 12
#define TWO_TOWERS_ITEM_NONE 0xff
#define LEGACY_ITEM_POOL_MAGIC 0x474c5454 // "TTLG"
#define LEGACY_ITEM_POOL_HEADER_SIZE 8
#define LEGACY_ITEMS_NAME_LENGTH 64
#define ITEM_RECORD_STATS 4

// A ROTK stat a Two Towers stat adds to, or STAT_NONE for one with no ROTK counterpart. Weapon
// damage uses the template's damage stat instead.
static const u8 StatMap[] = {
    STAT_NONE,
    STAT_DAMAGE_TWOHAND,
    STAT_DAMAGE_FIRE,
    STAT_DAMAGE_FLAT,
    STAT_DAMAGE_FLAT,
    STAT_DAMAGE_TO_ANIMALS,
    STAT_DAMAGE_TO_CREBAIN,
    STAT_DAMAGE_TO_WARGS,
    STAT_DAMAGE_TO_ORCS,
    STAT_DAMAGE_TO_URUKHAI,
    STAT_DAMAGE_TO_NAZGUL,
    STAT_THORNS_DAMAGE,
    STAT_NONE, // two-handed flag
    STAT_MELEE_ARMOR,
    STAT_MISSILE_ARMOR,
    STAT_FIRE_PROTECTION,
    STAT_CRITICAL_ARMOR,
    STAT_NONE, // day armor
    STAT_NONE, // night armor
    STAT_STRENGTH,
    STAT_ACCURACY,
    STAT_HEALTH,
    STAT_DEFENSE,
    STAT_COURAGE,
    STAT_MAX_HP,
    STAT_MAX_SPIRIT,
    STAT_CRIT_CHANCE_PERCENT,
    STAT_SPEED_PERCENT,
    STAT_GEMFINDER,
    STAT_HP_PER_KILL,
    STAT_NONE, // health percent per hit
    STAT_SPIRIT_REGEN,
    STAT_HP_REGEN,
    STAT_EXTRA_HP_FROM_FOOD_PERCENT,
    STAT_SPIRIT_PER_HIT,
    STAT_FOE_STUN_TIME,
    STAT_EXTRA_EXP_PERCENT,
    STAT_CORRUPTION_RESIST_PERCENT,
    STAT_PARRY_PERCENT,
    STAT_BLOCK_PERCENT,
    STAT_RIPOSTE_PERCENT,
    STAT_EXTRA_TREASURE_PERCENT,
    STAT_KNOCKBACK,
    STAT_NONE, // spell damage
    STAT_NONE, // the 8 Two Towers skill bonuses apply to legacy skills
    STAT_NONE,
    STAT_NONE,
    STAT_NONE,
    STAT_NONE,
    STAT_NONE,
    STAT_NONE,
    STAT_NONE,
    STAT_ALL_PRIMARY_STATS,
    STAT_NONE, // all passive skills
    STAT_NONE, // all active skills
    STAT_NONE, // all skills
};

// The ROTK item a Two Towers base item becomes: its type, and the base item whose icon and equip
// flags it borrows.
typedef struct LegacyBase
{
    u8 itemType;
    u8 templateBase;
} LegacyBase;

static const LegacyBase LegacyBases[] = {
    {ITEM_TYPE_BOW, 0},       {ITEM_TYPE_BOW, 2},        {ITEM_TYPE_BOW, 7},        {ITEM_TYPE_BOW, 9},
    {ITEM_TYPE_BOW, 10},      {ITEM_TYPE_ARROW, 5},      {ITEM_TYPE_ARROW, 6},      {ITEM_TYPE_ARROW, 3},
    {ITEM_TYPE_ARROW, 1},     {ITEM_TYPE_ARROW, 7},      {ITEM_TYPE_ARROW, 8},      {ITEM_TYPE_ARROW, 9},
    {ITEM_TYPE_KNIFE, 6},     {ITEM_TYPE_KNIFE, 12},     {ITEM_TYPE_KNIFE, 15},     {ITEM_TYPE_KNIFE, 16},
    {ITEM_TYPE_SWORD_1H, 1},  {ITEM_TYPE_SWORD_1H, 9},   {ITEM_TYPE_SWORD_1H, 7},   {ITEM_TYPE_KNIFE, 8},
    {ITEM_TYPE_SWORD_1H, 0},  {ITEM_TYPE_SWORD_1H, 5},   {ITEM_TYPE_SWORD_1H, 3},   {ITEM_TYPE_SWORD_1H, 4},
    {ITEM_TYPE_SWORD_1H, 2},  {ITEM_TYPE_SWORD_2H, 3},   {ITEM_TYPE_SWORD_2H, 4},   {ITEM_TYPE_SWORD_1H, 10},
    {ITEM_TYPE_SWORD_1H, 0},  {ITEM_TYPE_SWORD_2H, 5},   {ITEM_TYPE_SWORD_2H, 7},   {ITEM_TYPE_SWORD_2H, 6},
    {ITEM_TYPE_AXE, 0},       {ITEM_TYPE_AXE, 1},        {ITEM_TYPE_AXE, 5},        {ITEM_TYPE_AXE, 10},
    {ITEM_TYPE_STAFF, 0},     {ITEM_TYPE_STAFF, 1},      {ITEM_TYPE_STAFF, 2},      {ITEM_TYPE_STAFF, 3},
    {ITEM_TYPE_KNIFE, 5},     {ITEM_TYPE_KNIFE, 0},      {ITEM_TYPE_KNIFE, 5},      {ITEM_TYPE_KNIFE, 5},
    {ITEM_TYPE_KNIFE, 3},     {ITEM_TYPE_KNIFE, 2},      {ITEM_TYPE_KNIFE, 14},     {ITEM_TYPE_ARMOR, 0},
    {ITEM_TYPE_ARMOR, 3},     {ITEM_TYPE_ARMOR, 4},      {ITEM_TYPE_ARMOR, 9},      {ITEM_TYPE_ARMOR, 10},
    {ITEM_TYPE_ARMOR, 12},    {ITEM_TYPE_ARMOR, 13},     {ITEM_TYPE_ARMOR, 15},     {ITEM_TYPE_ARMOR, 16},
    {ITEM_TYPE_ARMOR, 18},    {ITEM_TYPE_ARMOR, 19},     {ITEM_TYPE_ARMOR, 21},     {ITEM_TYPE_CLOAK, 0},
    {ITEM_TYPE_CLOAK, 1},     {ITEM_TYPE_CLOAK, 3},      {ITEM_TYPE_CLOAK, 5},      {ITEM_TYPE_CLOAK, 6},
    {ITEM_TYPE_SHIELD, 0},    {ITEM_TYPE_SHIELD, 1},     {ITEM_TYPE_SHIELD, 4},     {ITEM_TYPE_SHIELD, 5},
    {ITEM_TYPE_SHIELD, 6},    {ITEM_TYPE_SHIELD, 7},     {ITEM_TYPE_SHOES, 1},      {ITEM_TYPE_SHOES, 3},
    {ITEM_TYPE_SHOES, 4},     {ITEM_TYPE_SHOES, 6},      {ITEM_TYPE_ACCESSORY, 10}, {ITEM_TYPE_ACCESSORY, 9},
    {ITEM_TYPE_ACCESSORY, 0}, {ITEM_TYPE_ACCESSORY, 5},  {ITEM_TYPE_ACCESSORY, 6},  {ITEM_TYPE_ACCESSORY, 3},
    {ITEM_TYPE_ACCESSORY, 2}, {ITEM_TYPE_ACCESSORY, 7},  {ITEM_TYPE_ARMOR, 8},      {ITEM_TYPE_ARMOR, 2},
    {ITEM_TYPE_ARMOR, 5},     {ITEM_TYPE_ARMOR, 7},      {ITEM_TYPE_ARMOR, 11},     {ITEM_TYPE_ARMOR, 20},
    {ITEM_TYPE_UNIQUE, 0},    {ITEM_TYPE_UNIQUE, 2},     {ITEM_TYPE_UNIQUE, 3},     {ITEM_TYPE_SWORD_1H, 0},
    {ITEM_TYPE_BOW, 7},       {ITEM_TYPE_BOW, 0},        {ITEM_TYPE_SWORD_1H, 0},   {ITEM_TYPE_UNIQUE, 6},
    {ITEM_TYPE_UNIQUE, 7},    {ITEM_TYPE_SWORD_1H, 0},   {ITEM_TYPE_UNIQUE, 18},    {ITEM_TYPE_UNIQUE, 11},
    {ITEM_TYPE_UNIQUE, 12},   {ITEM_TYPE_UNIQUE, 13},    {ITEM_TYPE_UNIQUE, 14},    {ITEM_TYPE_UNIQUE, 15},
    {ITEM_TYPE_UNIQUE, 16},   {ITEM_TYPE_UNIQUE, 17},    {ITEM_TYPE_BOW, 7},        {ITEM_TYPE_BOW, 7},
    {ITEM_TYPE_BOW, 0},       {ITEM_TYPE_UNIQUE, 26},    {ITEM_TYPE_SWORD_1H, 0},   {ITEM_TYPE_SWORD_1H, 0},
    {ITEM_TYPE_UNIQUE, 29},   {ITEM_TYPE_AXE, 1},        {ITEM_TYPE_AXE, 1},        {ITEM_TYPE_STAFF, 3},
    {ITEM_TYPE_STAFF, 3},     {ITEM_TYPE_STAFF, 3},      {ITEM_TYPE_ARMOR, 12},     {ITEM_TYPE_ARMOR, 12},
    {ITEM_TYPE_ARMOR, 12},    {ITEM_TYPE_CLOAK, 3},      {ITEM_TYPE_CLOAK, 3},      {ITEM_TYPE_CLOAK, 1},
    {ITEM_TYPE_ARMOR, 8},     {ITEM_TYPE_ARMOR, 14},     {ITEM_TYPE_ARMOR, 2},
};

// A Two Towers item: two prefixes, a base item and a suffix, TWO_TOWERS_ITEM_NONE for none.
typedef union TwoTowersItem {
    u32 word;
    struct
    {
        u8 prefixA;
        u8 prefixB;
        u8 baseIndex;
        u8 suffixIndex;
    } d;
} TwoTowersItem;

// Bookkeeping in the PC host's legacy scratch (host RAM, zeroed at boot).
typedef struct LegacyItemsScratch
{
    // per legacy row: its legacy item pool index plus one, 0 for a free row
    u16 poolEntry[LEGACY_ITEM_ROW_COUNT];
    // per legacy row: the Two Towers item it shows
    TwoTowersItem item[LEGACY_ITEM_ROW_COUNT];
    // a row's two prefix names, put together for legacyItems_findString
    char prefixName[LEGACY_ITEMS_NAME_LENGTH];
} LegacyItemsScratch;

asm(".section .text\n"
    ".balign 4\n"
    ".thumb\n"
    ".global legacyItems_original_serializePlayer\n"
    ".thumb_func\n"
    "legacyItems_original_serializePlayer:\n"
    "    push {r4-r7, lr}\n"
    "    mov  r7, r10\n"
    "    mov  r6, r9\n"
    "    mov  r5, r8\n"
    "    ldr  r3, =save_serializePlayer + 8\n"
    "    bx   r3\n"
    "    .pool\n"
    ".balign 4\n"
    ".global legacyItems_original_deserializePlayer\n"
    ".thumb_func\n"
    "legacyItems_original_deserializePlayer:\n"
    "    push {r4-r7, lr}\n"
    "    mov  r7, r10\n"
    "    mov  r6, r9\n"
    "    mov  r5, r8\n"
    "    ldr  r3, =save_deserializePlayer + 8\n"
    "    bx   r3\n"
    "    .pool\n");

void legacyItems_original_serializePlayer(u16 *slotBuffer, u32 mission);
void legacyItems_original_deserializePlayer(u32 *slotBuffer, u32 playerIndex);

static LegacyItemsScratch *scratch(void)
{
    const volatile PcHostInfo *info = hackLib_pcHostInfo();
    if (info == NULL || info->version < 4 || info->legacyScratch == 0 || info->itemTypeInfos == 0)
    {
        return NULL;
    }
    return (LegacyItemsScratch *)info->legacyScratch;
}

static ItemTypeInfo *typeInfo(u32 itemType)
{
    return (ItemTypeInfo *)hackLib_pcHostInfo()->itemTypeInfos + itemType;
}

static void copyWords(void *dst, const void *src, u32 bytes)
{
    u32 *to = dst;
    const u32 *from = src;
    for (u32 i = 0; i < bytes / 4; i++)
    {
        to[i] = from[i];
    }
}

static const u8 *twoTowersRecord(const u8 *table, u32 count, u32 index)
{
    return index < count ? table + index * TWO_TOWERS_RECORD_SIZE : NULL;
}

static u16 recordNameId(const u8 *record)
{
    return record[0] | (record[1] << 8);
}

// The record's 4 stat and value pairs as bytes.
static u8 *statPairs(ItemStatRecord *record)
{
    return (u8 *)record + __builtin_offsetof(ItemStatRecord, stat0);
}

static void clearRecord(ItemStatRecord *record, u16 nameId)
{
    u8 *pairs = statPairs(record);
    record->nameId = nameId;
    record->tier = 0;
    for (u32 i = 0; i < ITEM_RECORD_STATS; i++)
    {
        pairs[i * 2] = STAT_NONE;
        pairs[i * 2 + 1] = 0;
    }
}

static void addStat(ItemStatRecord *record, u32 stat, s32 value)
{
    u8 *pairs = statPairs(record);
    if (stat == STAT_NONE || value == 0)
    {
        return;
    }
    for (u32 i = 0; i < ITEM_RECORD_STATS; i++)
    {
        if (pairs[i * 2] == stat || pairs[i * 2] == STAT_NONE)
        {
            pairs[i * 2] = stat;
            pairs[i * 2 + 1] = (s8)pairs[i * 2 + 1] + value;
            return;
        }
    }
}

// Add a Two Towers record's stats, weapon damage as damageStat.
static void addRecordStats(ItemStatRecord *record, const u8 *source, u32 damageStat)
{
    for (u32 i = 0; i < TWO_TOWERS_RECORD_STATS; i++)
    {
        u32 stat = source[4 + i * 2];
        s32 value = (s8)source[5 + i * 2];
        if (stat == TWO_TOWERS_STAT_DAMAGE)
        {
            addStat(record, damageStat, value);
        }
        else if (stat < ARRAY_COUNT(StatMap))
        {
            addStat(record, StatMap[stat], value);
        }
    }
}

static bool32 isTwoHanded(const u8 *source)
{
    for (u32 i = 0; i < TWO_TOWERS_RECORD_STATS; i++)
    {
        if (source[4 + i * 2] == TWO_TOWERS_STAT_TWO_HANDED && source[5 + i * 2] != 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// Build legacy row `row` of the item's type from a Two Towers item, returning the ROTK item.
static bool32 buildRows(Item *out, u32 row, TwoTowersItem source)
{
    const TwoTowersData *data = hackLib_twoTowersData();
    const u8 *base = twoTowersRecord(data->baseItems, data->baseItemCount, source.d.baseIndex);
    const u8 *prefixA = twoTowersRecord(data->prefixes, data->prefixCount, source.d.prefixA);
    const u8 *prefixB = twoTowersRecord(data->prefixes, data->prefixCount, source.d.prefixB);
    const u8 *suffix = twoTowersRecord(data->suffixes, data->suffixCount, source.d.suffixIndex);
    if (base == NULL || source.d.baseIndex >= ARRAY_COUNT(LegacyBases))
    {
        return FALSE;
    }

    const LegacyBase *legacy = &LegacyBases[source.d.baseIndex];
    ItemTypeInfo *info = typeInfo(legacy->itemType);
    const ItemStatRecord *template = &info->baseItems[legacy->templateBase];
    u32 damageStat =
        template->stat0 == STAT_DAMAGE_IMPALE ? STAT_DAMAGE_IMPALE : STAT_DAMAGE_SLASH;

    ItemStatRecord *baseRow = (ItemStatRecord *)&info->baseItems[row];
    clearRecord(baseRow, LEGACY_ITEMS_TEXT_ID_BASE + recordNameId(base));
    addRecordStats(baseRow, base, damageStat);
    copyWords((ItemGfx *)&info->baseGfx[row], &info->baseGfx[legacy->templateBase],
              sizeof(ItemGfx));

    ItemMetadata *metadata = (ItemMetadata *)&info->baseMetadata[row];
    copyWords(metadata, &info->baseMetadata[legacy->templateBase], sizeof(ItemMetadata));
    metadata->flags.p &= ~(ITEM_FLAG_NO_PREFIX | ITEM_FLAG_NO_SUFFIX);
    if (isTwoHanded(base))
    {
        metadata->flags.p = (metadata->flags.p & ~ITEM_FLAG_SLOT_OFFHAND) | ITEM_FLAG_TWO_HANDED;
    }
    else if (metadata->flags.p & ITEM_FLAG_TWO_HANDED)
    {
        metadata->flags.p = (metadata->flags.p & ~ITEM_FLAG_TWO_HANDED) | ITEM_FLAG_SLOT_OFFHAND;
    }

    out->d.itemType = legacy->itemType;
    out->d.runeIndex = RUNE_NONE;
    out->d.baseIndex = row;
    out->d.prefixIndex = ITEM_AFFIX_NONE;
    out->d.suffixIndex = ITEM_AFFIX_NONE;

    // Types without an affix table carry the affix stats on the base row.
    if (prefixA != NULL || prefixB != NULL)
    {
        ItemStatRecord *prefixRow = baseRow;
        if (info->prefixes != NULL)
        {
            u32 nameId = LEGACY_ITEMS_PREFIX_TEXT_ID_BASE + row;
            if (prefixA == NULL || prefixB == NULL)
            {
                nameId = LEGACY_ITEMS_TEXT_ID_BASE + recordNameId(prefixA != NULL ? prefixA : prefixB);
            }
            prefixRow = (ItemStatRecord *)&info->prefixes[row];
            clearRecord(prefixRow, nameId);
            ((AffixFlags *)&info->prefixFlags[row])->p = 0;
            out->d.prefixIndex = row;
        }
        if (prefixA != NULL)
        {
            addRecordStats(prefixRow, prefixA, damageStat);
        }
        if (prefixB != NULL)
        {
            addRecordStats(prefixRow, prefixB, damageStat);
        }
    }
    if (suffix != NULL)
    {
        ItemStatRecord *suffixRow = baseRow;
        if (info->suffixes != NULL)
        {
            suffixRow = (ItemStatRecord *)&info->suffixes[row];
            clearRecord(suffixRow, LEGACY_ITEMS_TEXT_ID_BASE + recordNameId(suffix));
            ((AffixFlags *)&info->suffixFlags[row])->p = 0;
            out->d.suffixIndex = row;
        }
        addRecordStats(suffixRow, suffix, damageStat);
    }
    return TRUE;
}

static bool32 readPoolEntry(u32 index, TwoTowersItem *out)
{
    u32 header[2];
    modSave_copy((u8 *)header, SRAM_BASE + LEGACY_ITEM_POOL_OFF, sizeof(header));
    if (header[0] != LEGACY_ITEM_POOL_MAGIC || index >= (header[1] & 0xffff))
    {
        return FALSE;
    }
    modSave_copy((u8 *)&out->word,
                 SRAM_BASE + LEGACY_ITEM_POOL_OFF + LEGACY_ITEM_POOL_HEADER_SIZE + index * 4, 4);
    return TRUE;
}

static bool32 isHandle(Item item)
{
    return item.d.itemType == ITEM_TYPE_UNIQUE && item.d.runeIndex == RUNE_NONE &&
           item.d.baseIndex == LEGACY_ITEM_HANDLE_BASE;
}

static bool32 isLegacyRow(Item item)
{
    return ITEM_TYPE(item) != ITEM_TYPE_EMPTY && LEGACY_ITEM_IS_ROW(item);
}

void legacyItems_materialize(Item *items, u32 count)
{
    LegacyItemsScratch *state = scratch();
    if (state == NULL || hackLib_twoTowersData() == NULL)
    {
        return;
    }
    for (u32 i = 0; i < count; i++)
    {
        if (!isHandle(items[i]))
        {
            continue;
        }
        u32 pool = items[i].d.prefixIndex | (items[i].d.suffixIndex << 8);
        u32 row = LEGACY_ITEM_ROW_COUNT;
        for (u32 r = 0; r < LEGACY_ITEM_ROW_COUNT; r++)
        {
            if (state->poolEntry[r] == pool + 1)
            {
                row = r;
                break;
            }
            if (row == LEGACY_ITEM_ROW_COUNT && state->poolEntry[r] == 0)
            {
                row = r;
            }
        }
        TwoTowersItem source;
        Item built;
        if (row == LEGACY_ITEM_ROW_COUNT || !readPoolEntry(pool, &source) ||
            !buildRows(&built, LEGACY_ITEM_ROW_FIRST + row, source))
        {
            continue;
        }
        state->poolEntry[row] = pool + 1;
        state->item[row] = source;
        items[i] = built;
    }
}

void legacyItems_dematerialize(Item *items, u32 count)
{
    LegacyItemsScratch *state = scratch();
    if (state == NULL)
    {
        return;
    }
    for (u32 i = 0; i < count; i++)
    {
        if (!isLegacyRow(items[i]))
        {
            continue;
        }
        u32 pool = state->poolEntry[items[i].d.baseIndex - LEGACY_ITEM_ROW_FIRST];
        if (pool == 0)
        {
            continue;
        }
        pool--;
        items[i].d.itemType = ITEM_TYPE_UNIQUE;
        items[i].d.runeIndex = RUNE_NONE;
        items[i].d.baseIndex = LEGACY_ITEM_HANDLE_BASE;
        items[i].d.prefixIndex = pool & 0xff;
        items[i].d.suffixIndex = pool >> 8;
    }
}

static const char *twoTowersString(const TwoTowersData *data, u32 id)
{
    u32 language = text_getLanguage();
    if (id >= data->stringCount || language >= LANGUAGE_COUNT)
    {
        return NULL;
    }
    return data->strings[language][id];
}

static u16 prefixNameId(const TwoTowersData *data, u32 prefix)
{
    const u8 *record = twoTowersRecord(data->prefixes, data->prefixCount, prefix);
    return record != NULL ? recordNameId(record) : 0xffff;
}

const char *legacyItems_findString(s32 textId)
{
    const TwoTowersData *data = hackLib_twoTowersData();
    LegacyItemsScratch *state = scratch();
    if (data == NULL || state == NULL || textId < LEGACY_ITEMS_TEXT_ID_BASE)
    {
        return NULL;
    }
    if (textId < LEGACY_ITEMS_PREFIX_TEXT_ID_BASE)
    {
        return twoTowersString(data, textId - LEGACY_ITEMS_TEXT_ID_BASE);
    }
    u32 row = textId - LEGACY_ITEMS_PREFIX_TEXT_ID_BASE - LEGACY_ITEM_ROW_FIRST;
    if (row >= LEGACY_ITEM_ROW_COUNT)
    {
        return NULL;
    }
    const char *first = twoTowersString(data, prefixNameId(data, state->item[row].d.prefixA));
    const char *second = twoTowersString(data, prefixNameId(data, state->item[row].d.prefixB));
    if (first == NULL || second == NULL)
    {
        return first != NULL ? first : second;
    }
    char *dst = state->prefixName;
    char *end = dst + LEGACY_ITEMS_NAME_LENGTH - 1;
    while (*first != '\0' && dst < end)
    {
        *dst++ = *first++;
    }
    if (dst < end)
    {
        *dst++ = ' ';
    }
    while (*second != '\0' && dst < end)
    {
        *dst++ = *second++;
    }
    *dst = '\0';
    return state->prefixName;
}

/**
 * Serialize the active player, then store legacy items as their handles.
 *
 * REPLACES (entry hook): save_serializePlayer.
 */
void legacyItems_serializePlayer(u16 *slotBuffer, u32 mission)
{
    SaveSlot *slot = (SaveSlot *)slotBuffer;

    legacyItems_original_serializePlayer(slotBuffer, mission);
    legacyItems_dematerialize(slot->d.inventory.array, ARRAY_COUNT(slot->d.inventory.array));
}

/**
 * Deserialize a slot whose legacy item handles are turned into legacy rows first, so equipped
 * legacy items apply their stats. The caller's slot buffer is left unchanged.
 *
 * REPLACES (entry hook): save_deserializePlayer.
 */
void legacyItems_deserializePlayer(u32 *slotBuffer, u32 playerIndex)
{
    SaveSlot slot;
    u16 sum = 0;

    copyWords(&slot, slotBuffer, sizeof(slot));
    legacyItems_materialize(slot.d.inventory.array, ARRAY_COUNT(slot.d.inventory.array));
    slot.d.checksum = 0;
    for (u32 i = 0; i < ARRAY_COUNT(slot.halfwords); i++)
    {
        sum += slot.halfwords[i];
    }
    slot.d.checksum = -sum;
    legacyItems_original_deserializePlayer(slot.words, playerIndex);
}
