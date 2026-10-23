#include "types.h"

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

#define g_MissionStateArea (*(SpawnTable **)0x0300324c)

/**
 * Copy the 8-byte spawn records of a mission block into the state area.
 *
 * @romaddress 0x0800c6e0
 */
u8 *mission_script_copySpawnRecords(const SpawnTable *block, u8 *chunk)
{
    u16 i;
    u16 size;
    u16 count;
    SpawnTable *t;

    count = block->count;
    g_MissionStateArea->count = count;
    i = 0;
    if (count != 0)
    {
        do
        {
            t = g_MissionStateArea;
            t->recs[i] = block->recs[i];
            i++;
        } while (i < t->count);
    }
    size = block->size;
    g_MissionStateArea->size = size;
    return (u8 *)g_MissionStateArea + size;
}
