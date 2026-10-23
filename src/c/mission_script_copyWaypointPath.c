#include "libc.h"
#include "match_hacks.h"
#include "mission.h"

/**
 * Copy a waypoint path into the mission state and widen its point count.
 *
 * @return The destination cursor past the path.
 *
 * @romaddress 0x0800c750
 */
u8 *mission_script_copyWaypointPath(const MissionOffsetTable *section, u8 *dest, u8 pathIndex)
{
    const MissionWaypointPath *source =
        (const MissionWaypointPath *)((const u8 *)section + section->offsets[pathIndex]);
    MissionWaypointPath *path = (MissionWaypointPath *)dest;
    u32 count = (u8)source->count;
    u8 i;

    path->count = count;
    for (i = 0; i < count; i++)
    {
        // HACK: preserve the byte-address operand order and copy alignment.
        u32 offset = i * sizeof(MissionWaypointRecord);
        memcpy((MissionWaypointRecord *)((u32)path + offset + sizeof(path->count)),
               (const MissionWaypointRecord *)((u32)source + offset + sizeof(source->count)),
               sizeof(MissionWaypointRecord));
    }
    {
        MATCH_PIN(u32, size, "r1") = count * sizeof(MissionWaypointRecord);
        MATCH_PIN(u32, bytes, "r0");

        MATCH_FRESH(bytes, size + sizeof(path->count));
        dest += bytes;
    }
    return dest;
}
