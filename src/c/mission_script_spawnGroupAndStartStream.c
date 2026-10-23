#include "mission.h"

/**
 * Spawn the script's group actors and start its stream, each only when its slot needs an
 * explicit load.
 *
 * @romaddress 0x0800c4ac
 */
void mission_script_spawnGroupAndStartStream(u8 groupIndex, u8 streamIndex)
{
    if (mission_script_slotNeedsLoad(groupIndex))
    {
        mission_script_spawnGroup(groupIndex);
    }
    if (mission_script_slotNeedsLoad(streamIndex))
    {
        mission_script_runStream(streamIndex);
    }
}
