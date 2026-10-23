#include "mission.h"
#include "variables.h"

/**
 * Return whether the mission script slot must be spawned explicitly.
 *
 * @romaddress 0x0800c538
 */
bool mission_script_slotNeedsLoad(u8 slotIndex)
{
    if ((g_MissionFlags & 1) && slotIndex == 1)
    {
        return FALSE;
    }
    if (slotIndex == 0)
    {
        return FALSE;
    }
    return TRUE;
}
