#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u8 *t = base + 2;
    u16 o = *(u16 *)(t + a * 2);
    u8 *p = base + o;
    u8 *t2 = p + 2;
    o = *(u16 *)(t2 + b * 2);
    return p + o + 8;
}
