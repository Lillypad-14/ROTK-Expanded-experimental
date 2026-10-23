#define mission_script_copySpawnRecords hdr_dummy
#include "mission.h"
#undef mission_script_copySpawnRecords

typedef struct SpawnRec
{
    u32 a;
    u32 b;
} SpawnRec;

typedef struct SpawnTable
{
    u16 size;
    u16 count;
    SpawnRec recs[1];
} SpawnTable;

#define g_MissionState (*(SpawnTable **)0x0300324c)

/**
 * Copy the mission block's header and spawn records into the mission-script state area.
 *
 * @return state-area cursor past the copy (start of the group state)
 *
 * @romaddress 0x0800c6e0
 */
u8 *mission_script_copySpawnRecords(const u8 *blockIn)
{
    const SpawnTable *block = (const SpawnTable *)blockIn;
    u16 i;
    u16 size;
    u16 count;
    SpawnTable *t;

    g_MissionState->count = block->count;
    count = block->count;
    i = 0;
    if (count != 0)
    {
        do
        {
            t = g_MissionState;
            t->recs[i] = block->recs[i];
            i++;
        } while (i < t->count);
    }
    size = block->size;
    g_MissionState->size = size;
    return (u8 *)g_MissionState + size;
}
