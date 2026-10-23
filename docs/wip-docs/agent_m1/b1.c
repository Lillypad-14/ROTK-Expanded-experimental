#include "types.h"
extern u8 *g_MissionGroups;
u8 *mission_script_getActorParams(u8 a, u8 b)
{
    u8 *base = g_MissionGroups;
    u32 i = a * 2;
    u32 j = b * 2;
    u8 *p = base + *(u16 *)(base + 2 + i);
    return p + *(u16 *)(p + 2 + j) + 8;
}
