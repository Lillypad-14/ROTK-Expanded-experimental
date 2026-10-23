#include "patches/skill_expansion/skill_expansion_save_format.h"

u8 skillExpansion_saveByteSum(const SkillSaveRecord *record)
{
    const u8 *bytes = (const u8 *)record;
    u8 sum = 0;
    u32 i;

    for (i = 0; i < sizeof(*record); i++)
    {
        sum += bytes[i];
    }
    return sum;
}

const SkillSaveEntry *skillExpansion_findSaveEntry(const SkillSaveEntry *entries, u32 count, u16 id)
{
    u32 i;

    if (id != 0)
    {
        for (i = 0; i < count; i++)
        {
            if (entries[i].id == id)
            {
                return &entries[i];
            }
        }
    }
    return NULL;
}

static bool validEntries(const SkillSaveEntry *entries, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (entries[i].level > 5 || entries[i].cycle > 1 ||
            (entries[i].id != 0 && skillExpansion_findSaveEntry(entries, i, entries[i].id) != NULL))
        {
            return FALSE;
        }
    }
    return TRUE;
}

bool skillExpansion_isValidSave(const SkillSaveRecord *record, u8 heroId, u16 slotChecksum)
{
    return record->tag == SKILL_SAVE_TAG && record->version == SKILL_SAVE_VERSION &&
           record->heroId == heroId && record->slotChecksum == slotChecksum &&
           skillExpansion_saveByteSum(record) == 0 &&
           validEntries(record->actives, SKILL_SAVE_ACTIVE_COUNT) &&
           validEntries(record->extraActives, SKILL_SAVE_EXTRA_ACTIVE_COUNT) &&
           validEntries(record->passives, SKILL_SAVE_PASSIVE_COUNT);
}
