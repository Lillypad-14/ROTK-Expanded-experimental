#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    u8 *base;
    u8 *g;
    u8 *t;
    u8 *t2;
    u32 i;
    u32 j;
    if (groupIndex == 0 && actorIndex > 253) {
        u8 slot = 255 - actorIndex;
        return PLAYER(slot).ownerActor;
    }
    base = g_MissionGroups;
    i = groupIndex * 2;
    t = base + 2;
    g = base + *(u16 *)(t + i);
    j = actorIndex * 2;
    t2 = g + 2;
    g = g + *(u16 *)(t2 + j);
    return *(Actor **)g;
}
