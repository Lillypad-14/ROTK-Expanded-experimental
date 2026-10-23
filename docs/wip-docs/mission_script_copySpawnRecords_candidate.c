#include "mission.h"
#include "variables.h"

/**
 * Copy the mission header and spawn positions into the mission script state.
 *
 * @return The state cursor past the mission header.
 *
 * @romaddress 0x0800c6e0
 */
u8 *mission_script_copySpawnRecords(const u8 *block, const u8 *chunk)
{
    const MissionBlockHeader *header = (const MissionBlockHeader *)block;
    u16 i;
    u32 headerSize;

    (void)chunk;
    g_MissionScriptState->spawnRecordCount = header->spawnRecordCount;
    for (i = 0; i < g_MissionScriptState->spawnRecordCount; i++)
    {
        g_MissionScriptState->spawnRecords[i] = header->spawnRecords[i];
    }
    headerSize = header->headerSize;
    g_MissionScriptState->headerSize = headerSize;
    return (u8 *)g_MissionScriptState + headerSize;
}
