#include "mission.h"
#include "variables.h"

/**
 * Params record of one actor in a mission group.
 *
 * @romaddress 0x0800c5d0
 */
ActorMissionParams *mission_script_getActorParams(u8 groupIndex, u8 actorIndex)
{
    u8 *base = (u8 *)g_MissionGroups;
    u32 i = groupIndex * 2;
    u8 *t = base + 2;
    u8 *p = base + *(u16 *)(t + i);
    u32 j = actorIndex * 2;
    u8 *t2 = p + 2;

    return (ActorMissionParams *)(p + *(u16 *)(t2 + j) + 8);
}
