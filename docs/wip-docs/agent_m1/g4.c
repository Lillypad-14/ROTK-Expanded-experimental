#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    u8 *base;
    u8 *g;
    u8 *t;
    u32 i;
    u32 j;
    if (groupIndex == 0 && actorIndex > 253) {
        u8 slot = 255 - actorIndex;
        return PLAYER(slot).ownerActor;
    }
    i = groupIndex * 2;
    base = g_MissionGroups;
    t = base + 2;
    g = base + *(u16 *)(t + i);
    j = actorIndex * 2;
    t = g + 2;
    g = g + *(u16 *)(t + j);
    return *(Actor **)g;
}
