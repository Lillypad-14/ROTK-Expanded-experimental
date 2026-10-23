#include "actor.h"
#include "mission.h"
#include "variables.h"

/**
 * Spawn a spawn group's actors, skipping slots that already have a live actor.
 *
 * @romaddress 0x0800c0ec
 */
void mission_script_spawnGroup(u8 groupIndex)
{
    u8 *base = g_MissionGroups;
    u32 idx = groupIndex * 2;
    u8 *t = base + 2;
    u16 off = *(u16 *)(t + idx);
    u8 *group = base + off;
    u8 count;
    u8 i;

    count = *group;
    for (i = 0; i < count; i++)
    {
        u32 j = i * 2;
        u8 *t2 = group + 2;
        u16 off2 = *(u16 *)(t2 + j);
        u8 *rec = group + off2;

        if (*(Actor **)rec == NULL)
        {
            *(Actor **)rec = (*(ActorMissionConstructor (*)[51])ActorMissionConstructors)[*(u16 *)(rec + 8)](groupIndex, i);
            if (*(Actor **)rec != NULL)
            {
                (*(Actor **)rec)->recordGroup = groupIndex;
                (*(Actor **)rec)->recordIndex = i;
                (*(Actor **)rec)->flags.p |= ACTOR_FLAG_MISSION_SPAWNED;
            }
        }
    }
}
