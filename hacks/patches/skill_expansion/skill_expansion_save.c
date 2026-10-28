#if HACK_legacy_items
#include "patches/legacy_items/legacy_items.h"
#endif
#include "patches/skill_expansion/skill_expansion_save.h"
#include "patches/skill_expansion/skill_expansion.h"
#include "patches/skill_expansion/skill_expansion_coop.h"
#include "patches/skill_expansion/skill_expansion_save_format.h"

#if HACK_mod_save

#include "memory.h"
#include "multiplayer.h"
#include "patches/mod_save/save_layout.h"
#include "patches/mod_save/save_slots.h"
#include "patches/mod_save/save_sram.h"
#include "scene/saveLoadMenu.h"
#include "variables.h"
#if HACK_debug_cheats
#include "patches/debug_cheats/debug_cheats.h"
#endif
#if HACK_experimental
#include "patches/experimental/experimental.h"
#endif
#if HACK_loot_qol
#include "patches/loot_qol/loot_qol.h"
#endif
#if HACK_twists
#include "patches/twists/twists.h"
#endif
#if HACK_versus
#include "patches/versus/versus.h"
#endif

// SkillExpansionSaveRecord.tag of a record that holds data. Blank SRAM reads as 0xFF.
#define SAVE_RECORD_TAG 0x5e

#define QUICK_SELECT_SLOT_COUNT 3

// SkillSaveRecord.reservedHeader[0] bits: the character plays its hero's vanilla kit.
#define SKILL_SAVE_FLAG_VANILLA_KIT 0x01

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
        record.reservedHeader[0] = skillExpansion_players[p].vanillaKit ? SKILL_SAVE_FLAG_VANILLA_KIT : 0;
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
    skillExpansion_players[p].vanillaKit = FALSE;
    if (skillExpansion_isValidSave(&idRecord, PLAYER(p).heroId, slot->d.checksum))
    {
        skillExpansion_players[p].vanillaKit = (idRecord.reservedHeader[0] & SKILL_SAVE_FLAG_VANILLA_KIT) != 0;
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

        if (row >= SKILL_EXPANSION_ACTIVE_ROW_CAPACITY || row == ACTIVE_SKILL_NONE ||
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

// A player's page 2 record holds what the stock co-op slot exchange leaves out: the new rows'
// bought ranks, then their cycle flags, the new slots' bought ranks and the full quick-select rows,
// which the stock slot stores in 3 bits each, then the character's kit (vanillaKit).
#define PAGE2_RECORD_SIZE \
    (2 * SKILL_EXPANSION_NEW_ACTIVE_COUNT + SKILL_EXPANSION_NEW_PASSIVE_COUNT + QUICK_SELECT_SLOT_COUNT + 1)

// Multiplayer ID of the console whose global mod options a co-op session plays with.
#define SESSION_OPTIONS_HOST 0

#if HACK_loot_qol
#define LOOT_QOL_OPTION_BYTES sizeof(g_LootQolState)
#else
#define LOOT_QOL_OPTION_BYTES 0
#endif
#if HACK_debug_cheats
#define DEBUG_CHEATS_OPTION_BYTES sizeof(g_DebugCheatsState)
#else
#define DEBUG_CHEATS_OPTION_BYTES 0
#endif
#if HACK_twists
#define TWISTS_OPTION_BYTES TWISTS_STATE_SIZE
#else
#define TWISTS_OPTION_BYTES 0
#endif
#if HACK_experimental
#define EXPERIMENTAL_OPTION_BYTES sizeof(g_ExperimentalState)
#else
#define EXPERIMENTAL_OPTION_BYTES 0
#endif
#if HACK_versus
#define VERSUS_OPTION_BYTES sizeof(g_VersusState)
#else
#define VERSUS_OPTION_BYTES 0
#endif

// The global mod options that change how the game plays. Each console loads its own save's at
// boot, so a co-op client takes the host's for the session to simulate the same game.
#define SESSION_OPTIONS_SIZE \
    (LOOT_QOL_OPTION_BYTES + DEBUG_CHEATS_OPTION_BYTES + TWISTS_OPTION_BYTES + EXPERIMENTAL_OPTION_BYTES + \
     VERSUS_OPTION_BYTES)

// A console's record: its player's page 2 record, then its session options.
#define PEER_RECORD_SIZE (PAGE2_RECORD_SIZE + SESSION_OPTIONS_SIZE)

// Record bytes per SKILL_EXPANSION_MP_PEER_RECORD message, after the chunk index.
#define PEER_RECORD_CHUNK_SIZE (SKILL_EXPANSION_MP_DATA_SIZE - 1)
#define PEER_RECORD_CHUNK_COUNT                                                                    \
    ((PEER_RECORD_SIZE + PEER_RECORD_CHUNK_SIZE - 1) / PEER_RECORD_CHUNK_SIZE)
#define PEER_RECORD_ALL_CHUNKS ((1 << PEER_RECORD_CHUNK_COUNT) - 1)

_Static_assert(PEER_RECORD_CHUNK_COUNT <= 8, "peerRecordChunks has a bit per chunk");

// Frames the slot exchange waits for the peer's record before loading its slot without one.
#define PEER_RECORD_TIMEOUT_FRAMES 120

// The peer's quick-select rows from its record. The rest of the record goes straight into its
// player, where loading its slot keeps it.
static u8 peerQuickSelect[QUICK_SELECT_SLOT_COUNT];

// Bit n set: chunk n of the peer's record has arrived.
static u8 peerRecordChunks;

// Chunks of this console's record sent in the current slot exchange.
static u8 sentRecordChunks;

// Frames the current slot exchange has waited for the peer's record.
static u8 peerRecordWaitFrames;

/** Byte `index` of this console's session options. */
static u8 *sessionOptionByte(u32 index)
{
#if HACK_loot_qol
    if (index < LOOT_QOL_OPTION_BYTES)
    {
        return (u8 *)&g_LootQolState + index;
    }
    index -= LOOT_QOL_OPTION_BYTES;
#endif
#if HACK_debug_cheats
    if (index < DEBUG_CHEATS_OPTION_BYTES)
    {
        return (u8 *)&g_DebugCheatsState + index;
    }
    index -= DEBUG_CHEATS_OPTION_BYTES;
#endif
#if HACK_twists
    if (index < TWISTS_OPTION_BYTES)
    {
        return twists_stateBytes() + index;
    }
    index -= TWISTS_OPTION_BYTES;
#endif
#if HACK_experimental
    if (index < EXPERIMENTAL_OPTION_BYTES)
    {
        return (u8 *)&g_ExperimentalState + index;
    }
    index -= EXPERIMENTAL_OPTION_BYTES;
#endif
#if HACK_versus
    return (u8 *)&g_VersusState + index;
#else
    (void)index;
    return NULL;
#endif
}

/** Byte `index` of player p's record, with the quick-select rows in `quickSelect`. */
static u8 *recordByte(u8 p, u32 index, u8 *quickSelect)
{
    if (index >= PAGE2_RECORD_SIZE)
    {
        return sessionOptionByte(index - PAGE2_RECORD_SIZE);
    }
    if (index < SKILL_EXPANSION_NEW_ACTIVE_COUNT)
    {
        return skillExpansion_activePurchased(p, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + index);
    }
    index -= SKILL_EXPANSION_NEW_ACTIVE_COUNT;
    if (index < SKILL_EXPANSION_NEW_ACTIVE_COUNT)
    {
        return skillExpansion_activeCycleEnabled(p, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + index);
    }
    index -= SKILL_EXPANSION_NEW_ACTIVE_COUNT;
    if (index < SKILL_EXPANSION_NEW_PASSIVE_COUNT)
    {
        return (u8 *)skillExpansion_passivePurchased(p, HERO_PASSIVE_SKILL_COUNT + index);
    }
    index -= SKILL_EXPANSION_NEW_PASSIVE_COUNT;
    if (index < QUICK_SELECT_SLOT_COUNT)
    {
        return &quickSelect[index];
    }
    return &skillExpansion_players[p].vanillaKit;
}

void skillExpansion_peerRecordReceive(u8 sender, const u8 *data)
{
    u32 chunk = data[0];
    u32 i;

    if (chunk >= PEER_RECORD_CHUNK_COUNT)
    {
        return;
    }
    for (i = 0; i < PEER_RECORD_CHUNK_SIZE; i++)
    {
        u32 index = chunk * PEER_RECORD_CHUNK_SIZE + i;

        // Only the host's session options apply.
        if (index < PAGE2_RECORD_SIZE ||
            (index < PEER_RECORD_SIZE && sender == SESSION_OPTIONS_HOST))
        {
            *recordByte(sender, index, peerQuickSelect) = data[1 + i];
        }
    }
    peerRecordChunks |= 1 << chunk;
}

static void sendRecordChunk(u32 chunk)
{
    u8 p = multiplayer_getMultiPlayerId();
    u8 data[SKILL_EXPANSION_MP_DATA_SIZE];
    u32 i;

    memory_memset(data, 0, sizeof(data));
    data[0] = chunk;
    for (i = 0; i < PEER_RECORD_CHUNK_SIZE; i++)
    {
        u32 index = chunk * PEER_RECORD_CHUNK_SIZE + i;

        if (index < PEER_RECORD_SIZE)
        {
            data[1 + i] = *recordByte(p, index, PLAYER(p).quickSelectActiveSkills);
        }
    }
    skillExpansion_coopSend(SKILL_EXPANSION_MP_PEER_RECORD, data);
}

/**
 * Once the stock co-op slot exchange is done, send this console's record and wait for the peer's
 * before the stock tick loads the peer's slot.
 *
 * @return TRUE to skip the stock tick this frame
 */
bool32 skillExpansion_saveMenuTickHook(void)
{
    u32 linkMessages;

#if HACK_legacy_items
    if (legacyItems_saveMenuTick())
    {
        return TRUE;
    }
#endif

    if (skillExpansion_heroSelectTick())
    {
        return TRUE;
    }
    if (SCENE_ARG_SAVE_LOAD_STATE != SAVE_LOAD_STATE_COOP_SLOT_EXCHANGE)
    {
        // The peer's record can't arrive yet: it waits for this console's slot.
        peerRecordChunks = 0;
        sentRecordChunks = 0;
        peerRecordWaitFrames = 0;
        return FALSE;
    }
    linkMessages = multiplayer_getMessageMask();
    if ((linkMessages & MP_MSG_FLAG_PEER_READY) == 0 ||
        (linkMessages & MP_MSG_FLAG_LOCAL_READY) == 0)
    {
        return FALSE;
    }
    if (sentRecordChunks < PEER_RECORD_CHUNK_COUNT)
    {
        sendRecordChunk(sentRecordChunks++);
        return TRUE;
    }
    if (peerRecordChunks != PEER_RECORD_ALL_CHUNKS &&
        peerRecordWaitFrames < PEER_RECORD_TIMEOUT_FRAMES)
    {
        peerRecordWaitFrames++;
        return TRUE;
    }
    return FALSE;
}

/**
 * Keep the peer's new rows and slots from its record, or empty them when none arrived. A client
 * applies the host's session options, which came with the record.
 *
 * REPLACES (whole function): scene_saveLoadMenu_loadPeerSlot.
 */
void skillExpansion_loadPeerSlot(u32 playerIndex)
{
    u8 p = playerIndex;
    bool valid = peerRecordChunks == PEER_RECORD_ALL_CHUNKS;
    u8 quickSelect[QUICK_SELECT_SLOT_COUNT];
    u32 i;

    save_deserializePlayer(g_MpPeerSaveSlot.words, p);
    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        s8 *purchased = skillExpansion_passivePurchased(p, HERO_PASSIVE_SKILL_COUNT + i);

        *purchased = valid ? clampLevel(*purchased) : 0;
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;
        u8 *purchased = skillExpansion_activePurchased(p, row);
        u8 *cycleEnabled = skillExpansion_activeCycleEnabled(p, row);

        *purchased = valid ? clampLevel(*purchased) : 0;
        *cycleEnabled = valid ? *cycleEnabled != 0 : 1;
    }
    skillExpansion_players[p].vanillaKit = valid && skillExpansion_players[p].vanillaKit != 0;
    for (i = 0; i < QUICK_SELECT_SLOT_COUNT; i++)
    {
        quickSelect[i] = valid ? peerQuickSelect[i] : PLAYER(p).quickSelectActiveSkills[i];
    }
    save_loadSlotState(p);
    applyQuickSelect(p, quickSelect);
    g_LastSaveSlot = SCENE_ARG_SAVE_LOAD_SELECTED_SLOT;
#if HACK_experimental
    if (valid && p == SESSION_OPTIONS_HOST)
    {
        experimental_applyState();
    }
#endif
}

#endif
