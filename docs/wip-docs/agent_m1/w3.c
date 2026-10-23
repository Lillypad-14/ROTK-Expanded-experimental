#include "mission.h"
#include "variables.h"

typedef struct { s16 x; s16 y; s16 a; s16 b; } WpEntry;
void mission_script_getWaypoint(Vector2Int *out, u8 pathIndex, u8 waypointIndex)
{
    u8 *base = g_MissionWaypointPaths;
    u32 i = pathIndex * 2;
    u8 *t = base + 2;
    u8 *path = base + *(u16 *)(t + i);
    WpEntry *wp = (WpEntry *)(path + 4) + waypointIndex;
    Vector2Int v;
    v.x = wp->x;
    v.y = wp->y;
    *out = v;
}
