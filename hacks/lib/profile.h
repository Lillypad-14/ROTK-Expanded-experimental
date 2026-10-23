#pragma once

#include "types.h"

enum HackLibProfileSection
{
    HACKLIB_PROFILE_PLAYER = 0,
    HACKLIB_PROFILE_PASSIVES = 1,
    HACKLIB_PROFILE_HERO = 2,
    HACKLIB_PROFILE_TARGETS = 3,
    HACKLIB_PROFILE_EFFECTS = 4,
    HACKLIB_PROFILE_SUMMONS = 5,
    HACKLIB_PROFILE_FX = 6,
    HACKLIB_PROFILE_SECTION_COUNT = 7,
};

typedef struct HackLibProfileRecord
{
    u32 calls;
    u32 totalLines;
    u32 peakLines;
    u32 lastLines;
    u32 work;
} HackLibProfileRecord;

typedef struct HackLibProfileSpan
{
    u32 start;
    u8 section;
    bool active;
} HackLibProfileSpan;

bool hackLib_enableProfiler(bool enable);
void hackLib_resetProfiler(void);
HackLibProfileSpan hackLib_profileBegin(u8 section);
void hackLib_profileEnd(HackLibProfileSpan span);
void hackLib_profileWork(u8 section, u32 units);
bool hackLib_getProfile(u8 section, HackLibProfileRecord *out);
u32 hackLib_profileClock(void);
