#pragma once

// Unlock All: a mod_options page that grants the save-wide unlocks (bonus maps, Sam, Smeagol,
// and the chapter select hero paths) without earning them. Unlocking is one-way: a granted
// unlock is indistinguishable from an earned one, so the rows never clear bits.

#include "types.h"

// The patch's mod_options page entries (compiled when HACK_mod_options).
#include "patches/mod_options/mod_options.h"

#if HACK_chapter_select
#define UNLOCK_ALL_OPTION_COUNT 4
#else
#define UNLOCK_ALL_OPTION_COUNT 3
#endif
extern const struct ModOptionEntry UnlockAllOptions[UNLOCK_ALL_OPTION_COUNT];
