#pragma once

#include "types.h"

/** TRUE while the bonus-map save menu's hero select hands over to the hero select screen. */
bool32 heroGrid_active(void);

/**
 * The save menu draws a slot: in the bonus-map hero select, open the hero select screen instead.
 * Returns TRUE when the slot draw is handled.
 */
bool32 heroGrid_drawSlot(u8 slot);
