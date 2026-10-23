#include "mission.h"
#include "variables.h"

/**
 * Update a spawn record's live actor reference.
 *
 * @romaddress 0x0800c648
 */
void mission_script_setRecordActor(Actor *actor, u8 groupIndex, u8 recordIndex)
{
    u8 *group = g_MissionGroups;

    group += ((MissionOffsetTable *)group)->offsets[groupIndex];
    group += ((MissionOffsetTable *)group)->offsets[recordIndex];
    *(Actor **)group = actor;
}
