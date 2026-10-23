#include "mission.h"
#include "variables.h"

/**
 * Waypoint count of one path in the merged table.
 *
 * @romaddress 0x0800c674
 */
u8 mission_script_getWaypointCount(u8 pathIndex)
{
    u8 *paths = g_MissionWaypointPaths;
    u16 *offsets = (u16 *)(paths + 2);

    return paths[offsets[pathIndex]];
}
