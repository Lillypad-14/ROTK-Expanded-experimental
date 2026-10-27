#include "patches/skill_expansion/skill_expansion_save.c"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

PlayerGlobals g_PlayerGlobals;
SkillExpansionPlayer skillExpansion_players[2];
SaveSlot g_SaveSlots[SAVE_SLOT_COUNT];
SaveSlot g_MpPeerSaveSlot;
SceneState g_SceneCurrent;
u8 g_SaveSlotPage;
u8 g_LastSaveSlot;
static u8 chip[SRAM_SIZE];
static bool moved;
static u32 messageMask;
static u8 sent[8][SKILL_EXPANSION_MP_DATA_SIZE];
static u32 sentCount;

u32 multiplayer_getMessageMask(void)
{
    return messageMask;
}

s32 multiplayer_getMultiPlayerId(void)
{
    return 0;
}

void skillExpansion_coopSend(u8 kind, const u8 *data)
{
    assert(kind == SKILL_EXPANSION_MP_PEER_RECORD && sentCount < 8);
    memcpy(sent[sentCount++], data, SKILL_EXPANSION_MP_DATA_SIZE);
}

static void castNothing(Actor *playerActor)
{
    (void)playerActor;
}

void *memory_memset(void *destination, u32 value, size_t size)
{
    return memset(destination, value, size);
}

void modSave_copy(u8 *destination, const u8 *source, u32 size)
{
    uintptr_t first = (uintptr_t)SRAM_BASE;
    if ((uintptr_t)destination >= first && (uintptr_t)destination < first + SRAM_SIZE)
    {
        destination = chip + ((uintptr_t)destination - first);
    }
    if ((uintptr_t)source >= first && (uintptr_t)source < first + SRAM_SIZE)
    {
        source = chip + ((uintptr_t)source - first);
    }
    memcpy(destination, source, size);
}

u8 save_getSlotMission(SaveSlot *slot)
{
    return slot->d.mission;
}

void save_deserializePlayer(u32 *source, u32 p)
{
    (void)source;
    (void)p;
}

void save_loadSlotState(u32 p)
{
    (void)p;
}

u16 skillExpansion_legacyActiveId(u32 hero, u32 row)
{
    return hero == 2 && row < 12 && row != ACTIVE_SKILL_NONE ? 0x300 + row + 1 : 0;
}

u16 skillExpansion_activeId(u32 hero, u32 row)
{
    if (moved && (row == 0 || row == 7))
    {
        row = row == 0 ? 7 : 0;
    }
    return skillExpansion_legacyActiveId(hero, row);
}

u16 skillExpansion_passiveId(u32 hero, u32 slot)
{
    if (hero != 2 || slot >= 18)
    {
        return 0;
    }
    if (moved && (slot == 0 || slot == 9))
    {
        slot = slot == 0 ? 9 : 0;
    }
    return LegacyPassiveIds[2][slot];
}

/** Run a co-op slot exchange in which `deliver` passes this console's record on as the peer's. */
static u32 exchangeRecords(bool deliver)
{
    u32 frames = 0;
    u32 delivered = 0;

    SCENE_ARG_SAVE_LOAD_STATE = SAVE_LOAD_STATE_SLOT_SELECT;
    assert(!skillExpansion_saveMenuTickHook());
    SCENE_ARG_SAVE_LOAD_STATE = SAVE_LOAD_STATE_COOP_SLOT_EXCHANGE;
    sentCount = 0;
    messageMask = MP_MSG_FLAG_PEER_READY;
    // The stock exchange runs until both of its steps are done.
    assert(!skillExpansion_saveMenuTickHook());
    messageMask |= MP_MSG_FLAG_LOCAL_READY;
    while (skillExpansion_saveMenuTickHook())
    {
        frames++;
        for (; deliver && delivered < sentCount; delivered++)
        {
            skillExpansion_peerRecordReceive(1, sent[delivered]);
        }
    }
    assert(sentCount == PEER_RECORD_CHUNK_COUNT);
    return frames;
}

static void testPeerRecord(void)
{
    u32 i;

    PLAYER(0).heroId = 2;
    PLAYER(1).heroId = 2;
    PLAYER(1).activeSkills = skillExpansion_players[1].activeSkills;
    for (i = 0; i < SKILL_EXPANSION_ACTIVE_ROW_COUNT; i++)
    {
        skillExpansion_players[1].activeSkills[i].callback = castNothing;
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        *skillExpansion_activePurchased(0, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i) = i % 6;
        *skillExpansion_activeCycleEnabled(0, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i) = i & 1;
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        *skillExpansion_passivePurchased(0, HERO_PASSIVE_SKILL_COUNT + i) = 5 - i % 6;
    }
    PLAYER(0).quickSelectActiveSkills[0] = 9;
    PLAYER(0).quickSelectActiveSkills[1] = 1;
    PLAYER(0).quickSelectActiveSkills[2] = 12;

    // Each chunk takes a frame, then the exchange goes on once the peer's record is in.
    assert(exchangeRecords(TRUE) == PEER_RECORD_CHUNK_COUNT);
    skillExpansion_loadPeerSlot(1);
    for (i = 0; i < SKILL_EXPANSION_NEW_ACTIVE_COUNT; i++)
    {
        u32 row = SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + i;

        assert(*skillExpansion_activePurchased(1, row) == i % 6);
        assert(*skillExpansion_activeCycleEnabled(1, row) == (i & 1));
    }
    for (i = 0; i < SKILL_EXPANSION_NEW_PASSIVE_COUNT; i++)
    {
        assert(*skillExpansion_passivePurchased(1, HERO_PASSIVE_SKILL_COUNT + i) == 5 - i % 6);
    }
    assert(PLAYER(1).quickSelectActiveSkills[0] == 9);
    assert(PLAYER(1).quickSelectActiveSkills[1] == 1);
    assert(PLAYER(1).quickSelectActiveSkills[2] == 12);

    // A peer that sends no record loads with empty new rows and slots after the timeout.
    assert(exchangeRecords(FALSE) == PEER_RECORD_CHUNK_COUNT + PEER_RECORD_TIMEOUT_FRAMES);
    skillExpansion_loadPeerSlot(1);
    assert(*skillExpansion_activePurchased(1, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW + 1) == 0);
    assert(*skillExpansion_activeCycleEnabled(1, SKILL_EXPANSION_FIRST_NEW_ACTIVE_ROW) == 1);
    assert(*skillExpansion_passivePurchased(1, HERO_PASSIVE_SKILL_COUNT) == 0);
    puts("PASS: co-op page 2 record exchange and its timeout");
}

int main(void)
{
    SaveSlot slot = {0};
    SkillExpansionSaveRecord legacy = {.tag = SAVE_RECORD_TAG, .slotChecksum = 123};
    u8 quick[3] = {0};
    SkillSaveRecord *stable = (SkillSaveRecord *)(chip + SKILL_EXPANSION_ID_SAVE_OFF);

    memset(chip, SRAM_BLANK, sizeof(chip));
    PLAYER(0).heroId = 2;
    slot.d.mission = 0;
    slot.d.checksum = 123;
    *skillExpansion_activePurchased(0, 0) = 2;
    *skillExpansion_passivePurchased(0, 0) = 3;
    PLAYER(0).quickSelectActiveSkills[0] = 7;
    legacy.activeSkillLevelsPurchased[0] = 4;
    legacy.activeSkillCycleEnabled[0] = 1;
    legacy.passiveSkillLevelsPurchased[0] = 5;
    legacy.quickSelectActiveSkills[0] = 7;
    legacy.sum = -byteSum(&legacy);
    modSave_copy(recordAddress(0), (const u8 *)&legacy, sizeof(legacy));
    // A legacy load maps through frozen identities, even after a tree reorder.
    moved = TRUE;
    readSaveRecord(0, &slot, 0, quick);
    assert(*skillExpansion_activePurchased(0, 0) == 4);
    assert(*skillExpansion_activePurchased(0, 7) == 2);
    assert(*skillExpansion_passivePurchased(0, 0) == 5);
    assert(*skillExpansion_passivePurchased(0, 9) == 3);
    assert(quick[0] == 0);
    assert(stable->tag == SRAM_BLANK);
    PLAYER(0).quickSelectActiveSkills[0] = quick[0];
    skillExpansion_writeSaveRecord(&slot, 0);
    assert(skillExpansion_isValidSave(stable, 2, 123));
    // The next versioned load preserves identities after a second reorder.
    moved = FALSE;
    readSaveRecord(0, &slot, 0, quick);
    assert(*skillExpansion_activePurchased(0, 0) == 2);
    assert(*skillExpansion_activePurchased(0, 7) == 4);
    assert(*skillExpansion_passivePurchased(0, 0) == 3);
    assert(*skillExpansion_passivePurchased(0, 9) == 5);
    assert(quick[0] == 7);
    stable->version++;
    readSaveRecord(0, &slot, 0, quick);
    assert(*skillExpansion_activePurchased(0, 7) == 0);
    assert(*skillExpansion_passivePurchased(0, 9) == 0);
    slot.d.mission = SLOT_MISSION_EMPTY;
    skillExpansion_writeSaveRecord(&slot, 0);
    assert(stable->tag == 0);
    puts("PASS: legacy save migration, reordered trees, quick bindings and empty slots");
    testPeerRecord();
    return 0;
}
