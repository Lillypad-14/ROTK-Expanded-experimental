#include "mission.h"
#include "libc.h"

static inline u32 pathByteSize(u32 count)
{
    return 4 + count * sizeof(MissionWaypointRecord);
}

/**
 * Copy a waypoint path into the mission state and widen its point count.
 *
 * @return The destination cursor past the path.
 *
 * @romaddress 0x0800c750
 */
u8 *mission_script_copyWaypointPath(const MissionOffsetTable *section, u8 *dest, u8 pathIndex)
{
    const MissionWaypointPath *source = (const MissionWaypointPath *)((const u8 *)section + section->offsets[pathIndex]);
    MissionWaypointPath *path = (MissionWaypointPath *)dest;
    u32 count = (u8)source->count;
    u8 i;

    path->count = count;
    for (i = 0; i < count; i++)
    {
        u32 offset = i * sizeof(MissionWaypointRecord);
        memcpy((MissionWaypointRecord *)((u32)path + offset + 4), (const MissionWaypointRecord *)((u32)source + offset + 4), sizeof(MissionWaypointRecord));
    }
    dest += pathByteSize(count);
    return dest;
}
