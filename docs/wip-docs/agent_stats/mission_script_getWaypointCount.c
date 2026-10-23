#include "mission.h"
#include "variables.h"

/** @romaddress 0x0800c674 */
u8 mission_script_getWaypointCount(u8 pathIndex)
{
    u8 *paths = g_MissionWaypointPaths;
    u32 i = pathIndex * 2;
    u8 *p = paths + 2;

    return paths[*(u16 *)(p + i)];
}
