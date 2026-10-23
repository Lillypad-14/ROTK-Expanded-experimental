#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u8 *grp = (u8 *)(base + 2);
    u8 *p;
    grp = grp + a * 2;
    p = base + *(u16 *)grp;
    grp = p + 2 + b * 2;
    return p + *(u16 *)grp + 8;
}
