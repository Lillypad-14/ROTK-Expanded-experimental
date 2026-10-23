#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    if (groupIndex == 0 && actorIndex > 253)
        return PLAYER(255 - actorIndex).ownerActor;
    {
        u32 i = groupIndex * 2;
        u8 *t = g_MissionGroups + 2;
        u8 *base = g_MissionGroups;
        u8 *g = base + *(u16 *)(t + i);
        u32 j = actorIndex * 2;
        u8 *t2 = g + 2;
        g = g + *(u16 *)(t2 + j);
        return *(Actor **)g;
    }
}
