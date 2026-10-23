#include "mission.h"
#include "variables.h"

/**
 * Map a mission id to its region (enum RegionId) via RegionMissionRanges.
 *
 * @romaddress 0x08037a74
 */
u32 mission_getRegion(u32 missionId)
{
    u8 i;

    for (i = 0; i <= 27; i++)
    {
        if (missionId >= RegionMissionRanges[i].loMission &&
            missionId <= RegionMissionRanges[i].hiMission)
        {
            break;
        }
    }
    return i;
}
