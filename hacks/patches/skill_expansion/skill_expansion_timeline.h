#pragma once

#include "actor.h"
#include "lib/cast_timeline.h"

const HackLibCastTimeline *skillExpansion_getTimeline(u8 playerIndex, u8 row);
bool skillExpansion_prepareTimeline(u8 playerIndex, Actor *actor,
                                    const HackLibCastTimeline *definition);
void skillExpansion_tickTimeline(u8 playerIndex, Actor *actor);
void skillExpansion_clearTimeline(u8 playerIndex);
bool skillExpansion_isTimelineBusy(u8 playerIndex);
