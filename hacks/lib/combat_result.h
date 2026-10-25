#pragma once

#include "actor.h"

typedef struct HackLibDamageResult
{
    bool hit;
    bool killed;
    s32 damageDealt;
    s32 overkillDamage;
} HackLibDamageResult;

/** Capture immediately after hit resolution, before dispatching secondary effects. */
static inline HackLibDamageResult hackLib_damageResult(s32 hpBefore, s32 hpAfter, bool hit)
{
    HackLibDamageResult result = {.hit = hit};

    if (hit && hpBefore > 0)
    {
        result.killed = hpAfter <= 0;
        if (hpAfter < 0)
        {
            result.overkillDamage = -hpAfter;
            hpAfter = 0;
        }
        if (hpAfter < hpBefore)
        {
            result.damageDealt = hpBefore - hpAfter;
        }
    }
    return result;
}
