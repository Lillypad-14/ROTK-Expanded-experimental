#pragma once

#include "types.h"

// Legacy skills (PC edition): the 11 skills of a hero imported from The Two Towers, kept in the
// slot's legacy skill record. Passives apply as their closest ROTK stats.

// Add the legacy passive skills' stats of the hero the player loaded.
void legacySkills_applyPassives(u8 playerIndex);
