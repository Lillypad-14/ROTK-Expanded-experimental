#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u8 *t = base + 2;
    u8 *p = base + *(u16 *)(t + a * 2);
    t = p + 2;
    return p + *(u16 *)(t + b * 2) + 8;
}
