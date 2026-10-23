#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    u8 *base;
    u8 *g;
    if (groupIndex == 0 && actorIndex > 253)
        return PLAYER(255 - actorIndex).ownerActor;
    base = g_MissionGroups;
    g = base + *(u16 *)(base + 2 + groupIndex * 2);
    g = g + *(u16 *)(g + 2 + actorIndex * 2);
    return *(Actor **)g;
}
