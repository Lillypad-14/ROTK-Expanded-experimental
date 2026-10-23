#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u32 i = a * 2;
    u32 j = b * 2;
    u8 *t = base + 2;
    u32 o = *(u16 *)(t + i);
    u8 *p = base + o;
    u8 *t2 = p + 2;
    u32 o2 = *(u16 *)(t2 + j);
    return p + o2 + 8;
}
