#include "decompress.h"
#include "memory.h"
#include "match_hacks.h"
#include "mission.h"
#include "variables.h"

#define MISSION_SCRIPT_STATE_SIZE 0x2c00

/**
 * Load the shared and selected mission chunks and build the mission script state.
 *
 * @romaddress 0x0800c044
 */
void mission_script_loadMissionBlock(const u8 *block)
{
    const MissionBlockHeader *header;
    const MissionVariantTable *variants;
    MATCH_PIN(const u8 *, source, "r2");
    MATCH_PIN(u32, offset, "r0");
    u32 sharedSize;
    u8 *shared;
    u8 *chunk;
    u8 *cursor;

    memory_memset(g_MissionScriptState, 0, MISSION_SCRIPT_STATE_SIZE);
    header = (const MissionBlockHeader *)block;
    offset = header->headerSize;
    source = block + offset;
    MATCH_BARRIER(source);
    variants = (const MissionVariantTable *)source;
    offset = variants->chunks[0].offset;
    source = block + offset;
    sharedSize = decompress_chunk(source, g_VramCapture);
    shared = g_VramCapture;
    offset = variants->chunks[variants->variantChunks[g_MissionVariant]].offset;
    source = block + offset;
    decompress_chunk(source, shared + sharedSize);
    chunk = g_VramCapture + sharedSize;
    cursor = mission_script_copySpawnRecords(block, chunk);
    g_MissionGroups = cursor;
    cursor = mission_script_loadGroups(shared, chunk, cursor);
    g_MissionWaypointPaths = cursor;
    cursor = mission_script_loadWaypointPaths(shared, chunk, cursor);
    g_MissionStreams = cursor;
    mission_script_loadStreams(shared, chunk, cursor);
}
