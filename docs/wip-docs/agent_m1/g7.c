#include "mission.h"
#include "player.h"
#include "variables.h"

Actor *mission_script_getGroupActor(u8 groupIndex, u8 actorIndex)
{
    u8 *base;
    u16 *o;
    u8 *g;
    if (groupIndex == 0 && actorIndex > 253) {
        u8 slot = 255 - actorIndex;
        return PLAYER(slot).ownerActor;
    }
    base = g_MissionGroups;
    o = (u16 *)(base + 2);
    g = base + o[groupIndex];
    o = (u16 *)(g + 2);
    g = g + o[actorIndex];
    return *(Actor **)g;
}
