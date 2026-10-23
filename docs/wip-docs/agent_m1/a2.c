#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u16 *o1 = (u16 *)(base + 2);
    u8 *grp = base + o1[a];
    u16 *o2 = (u16 *)(grp + 2);
    return grp + o2[b] + 8;
}
