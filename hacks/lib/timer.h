#pragma once

#include "types.h"

/** Extend a timer without shortening it. Durations are measured in gameplay ticks. */
static inline bool hackLib_refreshTimer(u16 *timer, s32 frames)
{
    if (frames > 65535)
    {
        frames = 65535;
    }
    if (frames <= *timer)
    {
        return FALSE;
    }
    *timer = frames;
    return TRUE;
}

/** Return TRUE exactly when an active timer expires. */
static inline bool hackLib_tickTimer(u16 *timer)
{
    return *timer != 0 && --*timer == 0;
}
