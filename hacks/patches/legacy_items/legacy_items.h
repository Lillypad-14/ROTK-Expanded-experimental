#pragma once

#include "item.h"
#include "types.h"

// Legacy items (PC edition): items imported from Two Towers saves. The save keeps each as a
// handle (UNIQUE, base LEGACY_ITEM_HANDLE_BASE, a 16-bit legacy item pool index in the affix
// bytes). In memory a handle becomes a real item of the matching ROTK type whose base, prefix and
// suffix rows (LEGACY_ITEM_ROW_FIRST and up) the patch builds from the Two Towers records in the
// PC host's extended item tables.

#define LEGACY_ITEM_HANDLE_BASE 0xfe
#define LEGACY_ITEM_ROW_FIRST 0xc0
#define LEGACY_ITEM_ROW_COUNT (LEGACY_ITEM_HANDLE_BASE - LEGACY_ITEM_ROW_FIRST)
// Whether an item is a legacy row (not a handle).
#define LEGACY_ITEM_IS_ROW(item)                                                                   \
    ((item).d.baseIndex >= LEGACY_ITEM_ROW_FIRST &&                                                \
     (item).d.baseIndex < LEGACY_ITEM_ROW_FIRST + LEGACY_ITEM_ROW_COUNT)

// Text ids that name legacy rows: Two Towers text ids, and a row's two prefixes together.
#define LEGACY_ITEMS_TEXT_ID_BASE 0x5000
#define LEGACY_ITEMS_PREFIX_TEXT_ID_BASE 0x5800

// Turn legacy item handles into legacy rows.
void legacyItems_materialize(Item *items, u32 count);
// Turn legacy rows back into the handles a save stores.
void legacyItems_dematerialize(Item *items, u32 count);
// A legacy item name for a text id, or NULL.
const char *legacyItems_findString(s32 textId);
