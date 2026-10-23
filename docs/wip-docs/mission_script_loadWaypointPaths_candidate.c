#include "mission.h"
#include "variables.h"

/**
 * Merge shared and variant waypoint paths into the mission state.
 *
 * @return The state cursor past the waypoint paths.
 *
 * @romaddress 0x0800c2cc
 */
u8 *mission_script_loadWaypointPaths(const u8 *chunk0, const u8 *chunk, u8 *dest)
{
    const MissionOffsetTable *shared = NULL;
    const MissionOffsetTable *paths;
    u16 count = 0;
    u8 i;

    if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
    {
        shared = (const MissionOffsetTable *)(chunk0 + ((const MissionChunkHeader *)chunk0)->waypointSectionOffset);
        count = shared->count;
    }
    paths = (const MissionOffsetTable *)(chunk + ((const MissionChunkHeader *)chunk)->waypointSectionOffset);
    count += paths->count;
    ((MissionOffsetTable *)g_MissionWaypointPaths)->count = count;
    dest += mission_script_getAlignedHalfwordBytes(count + 1);
    if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
    {
        for (i = 0; i < shared->count; i++)
        {
            MissionOffsetTable *table = (MissionOffsetTable *)g_MissionWaypointPaths;
            u32 offset = dest - (u8 *)table;

            table->offsets[i] = offset;
            dest = mission_script_copyWaypointPath(shared, dest, i);
        }
        count = shared->count;
    }
    else
    {
        count = 0;
    }
    for (i = 0; i < paths->count; i++)
    {
        MissionOffsetTable *table = (MissionOffsetTable *)g_MissionWaypointPaths;
        u32 offset = dest - (u8 *)table;

        table->offsets[i + count] = offset;
        dest = mission_script_copyWaypointPath(paths, dest, i);
    }
    return dest;
}
