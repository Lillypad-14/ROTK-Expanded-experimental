#include "vector2.h"
extern u8 *g_MissionWaypointPaths;
Vector2Int mission_script_getWaypoint(u8 pathIndex, u8 waypointIndex)
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
    return v;
}
