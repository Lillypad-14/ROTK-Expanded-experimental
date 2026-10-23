#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    u8 *base;
    u8 *g;
    u32 i;
    u32 j;
    if (groupIndex == 0 && actorIndex > 253) {
        u8 slot = 255 - actorIndex;
        return PLAYER(slot).ownerActor;
    }
    i = groupIndex * 2;
    base = g_MissionGroups;
    g = base + *(u16 *)(base + 2 + i);
    j = actorIndex * 2;
    g = g + *(u16 *)(g + 2 + j);
    return *(Actor **)g;
}
