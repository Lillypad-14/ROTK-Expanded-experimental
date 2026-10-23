#include "mission.h"
#include "variables.h"

void mission_script_getWaypoint(Vector2Int *out, u8 pathIndex, u8 waypointIndex)
{
    u8 *base = g_MissionWaypointPaths;
    u32 i = pathIndex * 2;
    u8 *t = base + 2;
    u8 *path = base + *(u16 *)(t + i);
    u32 j = waypointIndex * 8;
    s16 *wp = (s16 *)(path + 4 + j);
    Vector2Int v;
    v.x = wp[0];
    v.y = wp[1];
    *out = v;
}
