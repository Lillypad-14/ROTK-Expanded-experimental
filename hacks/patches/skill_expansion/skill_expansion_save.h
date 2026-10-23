// Skill Expansion save records: the levels, cycle flags and quick-select rows the stock slot
// format has no room for, one record per save slot in mod_save's region 5.
#pragma once

#include "save.h"
#include "types.h"

/**
 * Write the active player's record for a slot that was just written, or a blank record for an
 * empty slot.
 *
 * @param slotNumber page * SAVE_SLOTS_PER_PAGE + on-page slot
 */
void skillExpansion_writeSaveRecord(const SaveSlot *slot, u32 slotNumber);
