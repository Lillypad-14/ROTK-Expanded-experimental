#include "mission.h"
#include "variables.h"

/**
 * Merge mission script streams and optionally prepend the shared chunk's first stream.
 *
 * @return The state cursor past the stream records.
 *
 * @romaddress 0x0800c3ac
 */
u8 *mission_script_loadStreams(const u8 *chunk0, const u8 *chunk, u8 *dest)
{
    u16 offset = ((const MissionChunkHeader *)chunk)->scriptSectionOffset;
    const MissionStreamSection *section = (const MissionStreamSection *)(chunk + offset);
    u32 count = section->count;
    u8 i;

    if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
    {
        g_MissionStreams->count = count + 1;
    }
    else
    {
        g_MissionStreams->count = count;
    }
    offset = mission_script_getAlignedHalfwordBytes(g_MissionStreams->count + 1);
    dest += offset;
    if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
    {
        const MissionChunkHeader *sharedHeader = (const MissionChunkHeader *)chunk0;

        g_MissionStreams->offsets[0] = offset;
        offset = sharedHeader->scriptSectionOffset;
        section = (const MissionStreamSection *)(chunk0 + offset);
        dest = mission_script_copyStream((const u8 *)section, dest, 0);
    }
    offset = ((const MissionChunkHeader *)chunk)->scriptSectionOffset;
    section = (const MissionStreamSection *)(chunk + offset);
    for (i = 0; i < count; i++)
    {
        offset = dest - (u8 *)g_MissionStreams;

        if (g_MissionFlags & MISSION_FLAG_SHARED_CHUNK)
        {
            g_MissionStreams->offsets[i + 1] = offset;
        }
        else
        {
            g_MissionStreams->offsets[i] = offset;
        }
        dest = mission_script_copyStream((const u8 *)section, dest, i);
    }
    return dest;
}
