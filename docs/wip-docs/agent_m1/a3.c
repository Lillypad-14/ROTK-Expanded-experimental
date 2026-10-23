#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u8 *grp = base + ((u16 *)(base + 2))[a];
    return grp + ((u16 *)(grp + 2))[b] + 8;
}
