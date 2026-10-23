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
    offset =
        mission_script_getAlignedHalfwordBytes(((MissionOffsetTable *)g_MissionGroups)->count + 1);
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

/**
 * Copy an actor group into runtime records with an aligned offset table.
 *
 * @return The destination cursor past the group.
 *
 * @romaddress 0x0800c248
 */
u8 *mission_script_copyGroup(const u8 *groupSection, u8 *dest, u8 groupIndex)
{
    const MissionGroupSection *section = (const MissionGroupSection *)groupSection;
    u16 size = section->offsets[groupIndex];
    const u8 *cursor = groupSection + size;
    const MissionActorGroupSource *source = (const MissionActorGroupSource *)cursor;
    MissionOffsetTable *group = (MissionOffsetTable *)dest;
    u32 count = source->count;
    u8 i;

    group->count = count;
    size = mission_script_alignByteSize(count + 2);
    cursor += size;
    size = mission_script_getAlignedHalfwordBytes(count + 1);
    for (i = 0; i < count; i++)
    {
        dest += size;
        group->offsets[i] = dest - (u8 *)group;
        size = source->recordSizes[i];
        mission_script_copyActorRecord(cursor, dest, size);
        cursor += size;
        size += sizeof(MissionActorRecordHeader);
    }
    dest += size;
    return dest;
}
