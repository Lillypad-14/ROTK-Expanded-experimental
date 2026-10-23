#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    u8 *base;
    u8 *g;
    u8 *t;
    if (groupIndex == 0 && actorIndex > 253) {
        u8 slot = 255 - actorIndex;
        return PLAYER(slot).ownerActor;
    }
    base = g_MissionGroups;
    t = base + 2;
    g = base + *(u16 *)(t + groupIndex * 2);
    t = g + 2;
    g = g + *(u16 *)(t + actorIndex * 2);
    return *(Actor **)g;
}
