#pragma once

#include "types.h"

// A level-up offer is an active skill row (learn it or raise it) or LEVEL_OFFER_STAT + a primary
// stat index (0 courage, 1 defense, 2 health, 3 accuracy, 4 strength).
#define LEVEL_OFFER_STAT 0x80
#define LEVEL_OFFER_NONE 0xff

/** Apply changed base stats, max HP included. */
void gauntletLevel_refreshStats(u8 playerIndex);

/** Spend the stock attribute points of new levels automatically and drop the skill points. */
void gauntletLevel_settle(u8 playerIndex);

/** Fill offers with distinct level-up choices, skills first. LEVEL_OFFER_NONE pads. */
void gauntletLevel_roll(u8 playerIndex, u8 offers[3]);

void gauntletLevel_apply(u8 playerIndex, u8 offer);

/** Write the offer's short label ("Learn X", "X Lv3", "+3 Health") to out. */
void gauntletLevel_label(u8 playerIndex, u8 offer, char *out);

/**
 * What the offer does, for the pick screen. A skill's text may hold '@1', which this sets to the
 * level the offer gives.
 */
const char *gauntletLevel_description(u8 playerIndex, u8 offer);

/** Current stat or skill rank and the value after taking this offer. Buffer: 48 bytes. */
void gauntletLevel_progress(u8 playerIndex, u8 offer, char *out);
