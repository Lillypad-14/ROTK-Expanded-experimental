#pragma once

#include "types.h"

// Legacy skills (PC edition): the 11 Two Towers skills of the six heroes who were in The Two Towers,
// kept in the slot's legacy skill record. A hero imported from The Two Towers arrives with its
// levels; the rest are bought with skill points. Passives apply as their closest ROTK stats.

// Add the legacy passive skills' stats of the hero the player loaded.
void legacySkills_applyPassives(u8 playerIndex);

#if PC_EDITION
// Fill the legacy active rows of the hero the player loaded, from its legacy skill record.
void legacySkills_buildRows(u8 heroId, u8 playerIndex);
// A save slot was written (saved TRUE) or erased: the slot keeps the loaded character's record.
void legacySkills_slotCommitted(bool32 saved);
// TRUE when the player plays a hero who has Two Towers skills.
bool32 legacySkills_loaded(u8 playerIndex);
// Buy a rank of a Two Towers skill with a skill point by The Two Towers' rules, with the legacy
// rows not swapped in. FALSE when it can't be bought.
bool32 legacySkills_buy(u32 skill);
// Its Two Towers skills: the passives first, then the actives (rows from
// SKILL_EXPANSION_FIRST_LEGACY_ACTIVE_ROW). skill counts over all 11.
u32 legacySkills_passiveCount(void);
u32 legacySkills_activeCount(void);
u8 legacySkills_level(u32 skill);
// A skill's icon (a ROTK skill icon asset), or NULL.
const void *legacySkills_icon(u32 skill);
// "Name (Level n) - description" in the game's language, or NULL.
const char *legacySkills_text(u32 skill);
#endif
