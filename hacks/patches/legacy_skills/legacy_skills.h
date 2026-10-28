#pragma once

#include "types.h"

// Legacy skills (PC edition): the 11 skills of a hero imported from The Two Towers, kept in the
// slot's legacy skill record. Passives apply as their closest ROTK stats.

// Add the legacy passive skills' stats of the hero the player loaded.
void legacySkills_applyPassives(u8 playerIndex);

#if PC_EDITION
// Fill the legacy active rows of the hero the player loaded, from its legacy skill record.
void legacySkills_buildRows(u8 heroId, u8 playerIndex);
// A save slot was written (saved TRUE) or erased: the slot keeps the loaded character's record.
void legacySkills_slotCommitted(bool32 saved);
#endif
