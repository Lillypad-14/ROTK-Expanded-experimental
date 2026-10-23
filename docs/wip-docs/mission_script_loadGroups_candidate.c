#include "mission.h"
#include "variables.h"

#define MISSION_GROUP_SECTION_OFFSET 8

/**
 * Merge mission spawn groups and optionally prepend the shared chunk's first group.
 *
 * @return The state cursor past the group records.
 *
 * @romaddress 0x0800c16c
 */
u8 *mission_script_loadGroups(const u8 *chunk0, const u8 *chunk, u8 *dest)
{
    const u8 *section;
    u32 count;
    u16 offset;
    u8 i;

    g_MissionFlags = ((const MissionChunkHeader *)chunk)->flags;
    chunk += MISSION_GROUP_SECTION_OFFSET;
    section = chunk;
    count = ((const MissionGroupSection *)chunk)->count;
    if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
    {
        ((MissionOffsetTable *)g_MissionGroups)->count = count + 1;
    }
    else
    {
        ((MissionOffsetTable *)g_MissionGroups)->count = count;
    }
    offset = mission_script_getAlignedHalfwordBytes(((MissionOffsetTable *)g_MissionGroups)->count + 1);
    dest += offset;
    if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
    {
        ((MissionOffsetTable *)g_MissionGroups)->offsets[0] = offset;
        section = chunk0 + MISSION_GROUP_SECTION_OFFSET;
        dest = mission_script_copyGroup(section, dest, 0);
    }
    section = chunk;
    for (i = 0; i < count; i++)
    {
        offset = dest - g_MissionGroups;
        if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
        {
            ((MissionOffsetTable *)g_MissionGroups)->offsets[i + 1] = offset;
        }
        else
        {
            ((MissionOffsetTable *)g_MissionGroups)->offsets[i] = offset;
        }
        dest = mission_script_copyGroup(section, dest, i);
    }
    return dest;
}
