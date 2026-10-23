#include "lib/profile.h"
#include "gba_io.h"
#include "memory.h"
#include "system.h"
#include "variables.h"

static HackLibProfileRecord *records;

bool hackLib_enableProfiler(bool enable)
{
    if (!enable)
    {
        if (records != NULL)
        {
            memory_free(records);
            records = NULL;
        }
        return TRUE;
    }
    if (records == NULL)
    {
        records = memory_zalloc(sizeof(*records) * HACKLIB_PROFILE_SECTION_COUNT);
    }
    return records != NULL;
}

void hackLib_resetProfiler(void)
{
    if (records != NULL)
    {
        memory_memset(records, 0, sizeof(*records) * HACKLIB_PROFILE_SECTION_COUNT);
    }
}

HackLibProfileSpan hackLib_profileBegin(u8 section)
{
    HackLibProfileSpan span = {0, section,
                               records != NULL && section < HACKLIB_PROFILE_SECTION_COUNT};
    if (span.active)
    {
        span.start = hackLib_profileClock();
    }
    return span;
}

void hackLib_profileEnd(HackLibProfileSpan span)
{
    if (span.active && records != NULL)
    {
        HackLibProfileRecord *record = &records[span.section];
        u32 lines = hackLib_profileClock() - span.start;
        record->calls++;
        record->totalLines += lines;
        record->lastLines = lines;
        if (lines > record->peakLines)
        {
            record->peakLines = lines;
        }
    }
}

void hackLib_profileWork(u8 section, u32 units)
{
    if (records != NULL && section < HACKLIB_PROFILE_SECTION_COUNT)
    {
        records[section].work += units;
    }
}

bool hackLib_getProfile(u8 section, HackLibProfileRecord *out)
{
    if (records == NULL || section >= HACKLIB_PROFILE_SECTION_COUNT)
    {
        return FALSE;
    }
    *out = records[section];
    return TRUE;
}
