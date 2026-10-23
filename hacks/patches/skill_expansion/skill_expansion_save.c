#include "patches/skill_expansion/skill_expansion_save.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_save_format.h"

#if HACK_mod_save

#include "memory.h"
#include "patches/mod_save/save_layout.h"
#include "patches/mod_save/save_slots.h"
#include "patches/mod_save/save_sram.h"
#include "scene/saveLoadMenu.h"
#include "variables.h"

// SkillExpansionSaveRecord.tag of a record that holds data. Blank SRAM reads as 0xFF.
#define SAVE_RECORD_TAG 0x5e

#define QUICK_SELECT_SLOT_COUNT 3

// New active rows in the row record. Later rows are only in the ID record.
#define ROW_RECORD_ACTIVE_COUNT 5

typedef struct SkillExpansionSaveRecord
{
    // SAVE_RECORD_TAG, or anything else for no data.
    u8 tag;

    // Makes the record's bytes sum to 0 mod 256.
    u8 sum;

    // SaveSlot.d.checksum of the slot this record was written with.
    u16 slotChecksum;

    // Passive slots 9..17.
    s8 passiveSkillLevelsPurchased[SKILL_EXPANSION_NEW_PASSIVE_COUNT];

    // Active rows 7..11.
    u8 activeSkillLevelsPurchased[ROW_RECORD_ACTIVE_COUNT];

    // Active rows 7..11.
    u8 activeSkillCycleEnabled[ROW_RECORD_ACTIVE_COUNT];

    // Player.quickSelectActiveSkills, which the stock slot stores in 3 bits each.
    u8 quickSelectActiveSkills[QUICK_SELECT_SLOT_COUNT];

    u8 reserved[6];
} SkillExpansionSaveRecord;

_Static_assert(sizeof(SkillExpansionSaveRecord) == SKILL_EXPANSION_SAVE_RECORD_SIZE,
               "save record size");

static const u8 DefaultQuickSelectRows[QUICK_SELECT_SLOT_COUNT] = {ACTIVE_SKILL_HERBAL_HEALING, 0,
                                                                   1};

_Static_assert(sizeof(SkillSaveRecord) == SKILL_EXPANSION_ID_SAVE_RECORD_SIZE,
               "skill ID save size");
_Static_assert(SKILL_SAVE_ACTIVE_COUNT + SKILL_SAVE_EXTRA_ACTIVE_COUNT ==
                   SKILL_EXPANSION_ACTIVE_ROW_COUNT,
               "saved active count");

static SkillSaveEntry *activeEntry(SkillSaveRecord *record, u32 row)
{
    return row < SKILL_SAVE_ACTIVE_COUNT ? &record->actives[row]
                                         : &record->extraActives[row - SKILL_SAVE_ACTIVE_COUNT];
}

static const SkillSaveEntry *findActiveEntry(const SkillSaveRecord *record, u16 id)
{
    const SkillSaveEntry *entry =
        skillExpansion_findSaveEntry(record->actives, SKILL_SAVE_ACTIVE_COUNT, id);

    return entry != NULL ? entry
                         : skillExpansion_findSaveEntry(record->extraActives,
                                                        SKILL_SAVE_EXTRA_ACTIVE_COUNT, id);
}
_Static_assert(SKILL_SAVE_PASSIVE_COUNT == SKILL_EXPANSION_PASSIVE_SLOT_COUNT,
               "saved passive count");

static u8 *idRecordAddress(u32 slotNumber)
{
    return SRAM_BASE + SKILL_EXPANSION_ID_SAVE_OFF +
           slotNumber * SKILL_EXPANSION_ID_SAVE_RECORD_SIZE;
}

static void writeIdRecord(const SaveSlot *slot, u32 slotNumber, u8 p)
{
    SkillSaveRecord record;
    u32 i;

    memory_memset(&record, 0, sizeof(record));
    if (save_getSlotMission((SaveSlot *)slot) < SLOT_MISSION_EMPTY)
    {
        record.tag = SKILL_SAVE_TAG;
        record.version = SKILL_SAVE_VERSION;
        record.heroId = PLAYER(p).heroId;
        record.slotChecksum = slot->d.checksum;
        for (i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
        {
            *activeEntry(&record, i) = (SkillSaveEntry){skillExpansion_activeId(record.heroId, i),
                                                 *skillExpansion_activePurchased(p, i),
                                                 *skillExpansion_activeCycleEnabled(p, i) != 0};
        }
        for (i = 0; i < SKILL_SAVE_PASSIVE_COUNT; i++)
        {
            record.passives[i] = (SkillSaveEntry){skillExpansion_passiveId(record.heroId, i),
                                                  *skillExpansion_passivePurchased(p, i), 0};
        }
        for (i = 0; i < SKILL_SAVE_QUICK_COUNT; i++)
        {
            record.quickSelect[i] =
                skillExpansion_activeId(record.heroId, PLAYER(p).quickSelectActiveSkills[i]);
        }
        record.sum = -skillExpansion_saveByteSum(&record);
    }
    modSave_copy(idRecordAddress(slotNumber), (const u8 *)&record, sizeof(record));
}

static u8 *recordAddress(u32 slotNumber)
{
    return SRAM_BASE + SKILL_EXPANSION_SAVE_OFF + slotNumber * SKILL_EXPANSION_SAVE_RECORD_SIZE;
}

static u8 byteSum(const SkillExpansionSaveRecord *record)
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

void skillExpansion_writeSaveRecord(const SaveSlot *slot, u32 slotNumber)
{
    u8 p = g_PlayerGlobals.activePlayerIndex;
    SkillExpansionSaveRecord record;
    u32 i;

    memory_memset(&record, SRAM_BLANK, sizeof(record));
    if (save_getSlotMission((SaveSlot *)slot) < SLOT_MISSION_EMPTY)
    {
        record.tag = SAVE_RECORD_TAG;
        record.sum = 0;
        record.slotChecksum = slot->d.checksum;
        for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
        {
            record.passiveSkillLevelsPurchased[i] =
                *skillExpansion_passivePurchased(p, HERO_PASSIVE_SKILL_COUNT + i);
        }
        for (i = 0; i < ROW_RECORD_ACTIVE_COUNT; i++)
        {
            u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;

            record.activeSkillLevelsPurchased[i] = *skillExpansion_activePurchased(p, row);
            record.activeSkillCycleEnabled[i] = *skillExpansion_activeCycleEnabled(p, row);
        }
        for (i = 0; i < QUICK_SELECT_SLOT_COUNT; i++)
        {
            record.quickSelectActiveSkills[i] = PLAYER(p).quickSelectActiveSkills[i];
        }
        record.sum = -byteSum(&record);
    }
    modSave_copy(recordAddress(slotNumber), (const u8 *)&record, sizeof(record));
    writeIdRecord(slot, slotNumber, p);
}

static u8 clampLevel(s32 level)
{
    if (level < 0 || level > PASSIVE_SKILL_MAX_LEVEL)
    {
        return 0;
    }
    return level;
}

static const u16 LegacyPassiveIds[9][SKILL_SAVE_PASSIVE_COUNT] = {
#include "skill_expansion_legacy_passives.inc"
};

static void migrateLegacyRecord(SkillSaveRecord *record, u8 p, const u8 *quickSelect)
{
    u32 heroId = PLAYER(p).heroId;
    u32 i;

    memory_memset(record, 0, sizeof(*record));
    for (i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
    {
        *activeEntry(record, i) = (SkillSaveEntry){skillExpansion_legacyActiveId(heroId, i),
                                              clampLevel(*skillExpansion_activePurchased(p, i)),
                                              *skillExpansion_activeCycleEnabled(p, i) != 0};
    }
    if (heroId < ARRAY_COUNT(LegacyPassiveIds))
    {
        for (i = 0; i < SKILL_SAVE_PASSIVE_COUNT; i++)
        {
            record->passives[i] = (SkillSaveEntry){
                LegacyPassiveIds[heroId][i], clampLevel(*skillExpansion_passivePurchased(p, i)), 0};
        }
    }
    for (i = 0; i < SKILL_SAVE_QUICK_COUNT; i++)
    {
        record->quickSelect[i] = skillExpansion_legacyActiveId(heroId, quickSelect[i]);
    }
}

static void applyIdRecord(const SkillSaveRecord *record, u8 p, u8 *quickSelect)
{
    u32 heroId = PLAYER(p).heroId;
    u32 i;

    for (i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
    {
        const SkillSaveEntry *entry = findActiveEntry(record, skillExpansion_activeId(heroId, i));
        *skillExpansion_activePurchased(p, i) = entry != NULL ? entry->level : 0;
        *skillExpansion_activeCycleEnabled(p, i) = entry != NULL ? entry->cycle : 1;
    }
    for (i = 0; i < SKILL_SAVE_PASSIVE_COUNT; i++)
    {
        const SkillSaveEntry *entry = skillExpansion_findSaveEntry(
            record->passives, SKILL_SAVE_PASSIVE_COUNT, skillExpansion_passiveId(heroId, i));
        *skillExpansion_passivePurchased(p, i) = entry != NULL ? entry->level : 0;
    }
    for (i = 0; i < SKILL_SAVE_QUICK_COUNT; i++)
    {
        u32 row;
        quickSelect[i] = DefaultQuickSelectRows[i];
        for (row = 0; row < SKILL_EXPANSION_ACTIVE_ROW_COUNT; row++)
        {
            if (record->quickSelect[i] != 0 &&
                skillExpansion_activeId(heroId, row) == record->quickSelect[i])
            {
                quickSelect[i] = row;
                break;
            }
        }
    }
}

/** Set the player's new-row and new-slot state from the slot's record, or clear it. */
static void readSaveRecord(u8 p, const SaveSlot *slot, u32 slotNumber, u8 *quickSelect)
{
    SkillExpansionSaveRecord record;
    bool valid;
    u32 i;
    SkillSaveRecord idRecord;

    modSave_copy((u8 *)&record, recordAddress(slotNumber), sizeof(record));
    valid = record.tag == SAVE_RECORD_TAG && byteSum(&record) == 0 &&
            record.slotChecksum == slot->d.checksum;

    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        *skillExpansion_passivePurchased(p, HERO_PASSIVE_SKILL_COUNT + i) =
            valid ? clampLevel(record.passiveSkillLevelsPurchased[i]) : 0;
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;
        bool inRecord = valid && i < ROW_RECORD_ACTIVE_COUNT;

        *skillExpansion_activePurchased(p, row) =
            inRecord ? clampLevel(record.activeSkillLevelsPurchased[i]) : 0;
        *skillExpansion_activeCycleEnabled(p, row) =
            inRecord ? record.activeSkillCycleEnabled[i] == 1 : 1;
    }
    for (i = 0; i < QUICK_SELECT_SLOT_COUNT; i++)
    {
        quickSelect[i] =
            valid ? record.quickSelectActiveSkills[i] : PLAYER(p).quickSelectActiveSkills[i];
    }
    modSave_copy((u8 *)&idRecord, idRecordAddress(slotNumber), sizeof(idRecord));
    if (skillExpansion_isValidSave(&idRecord, PLAYER(p).heroId, slot->d.checksum))
    {
        applyIdRecord(&idRecord, p, quickSelect);
    }
    else if (idRecord.tag != SKILL_SAVE_TAG)
    {
        // Legacy imports remain read-only until the player next saves.
        migrateLegacyRecord(&idRecord, p, quickSelect);
        applyIdRecord(&idRecord, p, quickSelect);
    }
    else
    {
        // A corrupt or newer record must not revive stale row-based expansion data.
        memory_memset(&idRecord, 0, sizeof(idRecord));
        for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
        {
            *skillExpansion_activePurchased(p, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i) = 0;
        }
        for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
        {
            *skillExpansion_passivePurchased(p, HERO_PASSIVE_SKILL_COUNT + i) = 0;
        }
    }
}

/** Quick-select rows from the record, with rows the hero doesn't have reset to the defaults. */
static void applyQuickSelect(u8 p, const u8 *quickSelect)
{
    u32 i;

    for (i = 0; i < QUICK_SELECT_SLOT_COUNT; i++)
    {
        u32 row = quickSelect[i];

        if (row >= SKILL_EXPANSION_ACTIVE_ROW_COUNT || row == ACTIVE_SKILL_NONE ||
            PLAYER(p).activeSkills[row].callback == NULL)
        {
            row = DefaultQuickSelectRows[i];
        }
        PLAYER(p).quickSelectActiveSkills[i] = row;
    }
}

/**
 * REPLACES (whole function): scene_saveLoadMenu_loadSlot.
 */
void skillExpansion_loadSlot(u32 playerIndex)
{
    u8 p = playerIndex;
    u32 slotIndex = SCENE_ARG_SAVE_LOAD_SELECTED_SLOT;
    u8 quickSelect[QUICK_SELECT_SLOT_COUNT];

    save_deserializePlayer(g_SaveSlots[slotIndex].words, p);
    readSaveRecord(p, &g_SaveSlots[slotIndex], g_SaveSlotPage * SAVE_SLOTS_PER_PAGE + slotIndex,
                   quickSelect);
    save_loadSlotState(p);
    applyQuickSelect(p, quickSelect);
    g_LastSaveSlot = slotIndex;
}

/**
 * The peer's record isn't sent over the link, so its new rows and slots load empty.
 *
 * REPLACES (whole function): scene_saveLoadMenu_loadPeerSlot.
 */
void skillExpansion_loadPeerSlot(u32 playerIndex)
{
    u8 p = playerIndex;
    u8 quickSelect[QUICK_SELECT_SLOT_COUNT];
    u32 i;

    save_deserializePlayer(g_MpPeerSaveSlot.words, p);
    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        *skillExpansion_passivePurchased(p, HERO_PASSIVE_SKILL_COUNT + i) = 0;
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;

        *skillExpansion_activePurchased(p, row) = 0;
        *skillExpansion_activeCycleEnabled(p, row) = 1;
    }
    for (i = 0; i < QUICK_SELECT_SLOT_COUNT; i++)
    {
        quickSelect[i] = PLAYER(p).quickSelectActiveSkills[i];
    }
    save_loadSlotState(p);
    applyQuickSelect(p, quickSelect);
    g_LastSaveSlot = SCENE_ARG_SAVE_LOAD_SELECTED_SLOT;
}

#endif
