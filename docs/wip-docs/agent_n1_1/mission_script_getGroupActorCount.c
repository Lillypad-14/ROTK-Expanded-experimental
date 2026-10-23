#define mission_script_getGroupActorCount hdr_dummy
#include "mission.h"
#include "variables.h"
#undef mission_script_getGroupActorCount

/**
 * Get the number of actors in a mission spawn group.
 *
 * @romaddress 0x0800c564
 */
u16 mission_script_getGroupActorCount(u8 groupIndex)
{
    u8 *groups = g_MissionGroups;
    u32 i = groupIndex * 2;
    u8 *p = groups + 2;

    return *(u16 *)(groups + *(u16 *)(p + i));
}
