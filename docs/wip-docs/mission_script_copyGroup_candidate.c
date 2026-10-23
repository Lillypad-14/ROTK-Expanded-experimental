#include "mission.h"

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
