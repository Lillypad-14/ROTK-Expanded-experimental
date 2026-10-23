#pragma once

#include "types.h"

#define SKILL_SAVE_TAG 0x73
#define SKILL_SAVE_VERSION 1
#define SKILL_SAVE_ACTIVE_COUNT 12
#define SKILL_SAVE_EXTRA_ACTIVE_COUNT 1
#define SKILL_SAVE_PASSIVE_COUNT 18
#define SKILL_SAVE_QUICK_COUNT 3

typedef struct SkillSaveEntry
{
    u16 id;
    u8 level;
    u8 cycle;
} SkillSaveEntry;

typedef struct SkillSaveRecord
{
    u8 tag;
    u8 version;
    u8 sum;
    u8 heroId;
    u16 slotChecksum;
    u8 reservedHeader[2];
    SkillSaveEntry actives[SKILL_SAVE_ACTIVE_COUNT];
    SkillSaveEntry passives[SKILL_SAVE_PASSIVE_COUNT];
    u16 quickSelect[SKILL_SAVE_QUICK_COUNT];
    // Active rows from SKILL_SAVE_ACTIVE_COUNT up.
    SkillSaveEntry extraActives[SKILL_SAVE_EXTRA_ACTIVE_COUNT];
    u8 reserved[6];
} SkillSaveRecord;

u8 skillExpansion_saveByteSum(const SkillSaveRecord *record);
bool skillExpansion_isValidSave(const SkillSaveRecord *record, u8 heroId, u16 slotChecksum);
const SkillSaveEntry *skillExpansion_findSaveEntry(const SkillSaveEntry *entries, u32 count,
                                                   u16 id);
