#define mission_script_copySpawnRecords hdr_dummy1
#define mission_script_loadGroups hdr_dummy2
#define mission_script_loadWaypointPaths hdr_dummy3
#define mission_script_loadStreams hdr_dummy4
#include "memory.h"
#include "mission.h"
#include "variables.h"
#undef mission_script_copySpawnRecords
#undef mission_script_loadGroups
#undef mission_script_loadWaypointPaths
#undef mission_script_loadStreams

u32 decompress_chunk(const u8 *src, u8 *dest);
u8 *mission_script_copySpawnRecords(const u8 *block, u8 *chunk);
u8 *mission_script_loadGroups(u8 *chunk0, u8 *chunk);
u8 *mission_script_loadWaypointPaths(u8 *chunk0, u8 *chunk);
u8 *mission_script_loadStreams(u8 *chunk0, u8 *chunk);

// Stand-in for the mission-script state area pointer at 0x0300324c.
#define g_MissionStateArea (*(void **)0x0300324c)

/**
 * Load a mission block: decompress the shared and variant-selected chunks and
 * build the stream and spawn-group state areas from them.
 *
 * @romaddress 0x0800c044
 */
void mission_script_loadMissionBlock(const u8 *block)
{
    const u8 *header;
    u32 size;
    u8 *chunk0;
    u8 *chunk;
    u8 *cursor;

    memory_memset(g_MissionStateArea, 0, 0x2c00);
    header = block + *(const u16 *)block;
    size = decompress_chunk(block + *(const u16 *)(header + 0xc), g_VramCapture);
    chunk0 = g_VramCapture;
    header += 8 * header[g_MissionVariant + 1];
    decompress_chunk(block + *(const u16 *)(header + 0xc), chunk0 + size);
    chunk = g_VramCapture + size;
    g_MissionGroups = mission_script_copySpawnRecords(block, chunk);
    g_MissionWaypointPaths = mission_script_loadGroups(chunk0, chunk);
    g_MissionStreams = mission_script_loadWaypointPaths(chunk0, chunk);
    mission_script_loadStreams(chunk0, chunk);
}
