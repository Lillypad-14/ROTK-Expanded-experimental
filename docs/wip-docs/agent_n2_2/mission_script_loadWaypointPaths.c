#define mission_script_loadWaypointPaths hdr_dummy
#include "mission.h"
#include "variables.h"
#undef mission_script_loadWaypointPaths

u16 unk_0800c7d8(u16 n);
u8 *unk_0800c750(const u16 *section, u8 *dest, u8 index);
asm(".global unk_0800c7d8\n.thumb_set unk_0800c7d8, 0x0800c7d8+1");
asm(".global unk_0800c750\n.thumb_set unk_0800c750, 0x0800c750+1");

/**
 * Merge both chunks' waypoint-path sections into the state area; sets
 * g_MissionWaypointPaths.
 *
 * @return state-area cursor past the table
 *
 * @romaddress 0x0800c2cc
 */
u8 *mission_script_loadWaypointPaths(const u8 *chunk0, const u8 *chunk, u8 *dest)
{
    const u16 *shared = NULL;
    u16 count = 0;
    const u16 *own;
    u8 i;

    if (g_MissionFlags & 1)
    {
        shared = (const u16 *)(chunk0 + *(u16 *)chunk0);
        count = *shared;
    }
    chunk += *(u16 *)chunk;
    own = (const u16 *)chunk;
    count = count + *own;
    *(u16 *)g_MissionWaypointPaths = count;
    dest += unk_0800c7d8(count + 1);
    if (g_MissionFlags & 1)
    {
        for (i = 0; i < *shared; i++)
        {
            { u8 *p = g_MissionWaypointPaths; u16 off = dest - p; p += 2; *(u16 *)(p + i * 2) = off; }
            dest = unk_0800c750(shared, dest, i);
        }
        count = *shared;
    }
    else
    {
        count = 0;
    }
    for (i = 0; i < *own; i++)
    {
        { u8 *p = g_MissionWaypointPaths; u16 off = dest - p; p += 2; *(u16 *)(p + (i + count) * 2) = off; }
        dest = unk_0800c750(own, dest, i);
    }
    return dest;
}
