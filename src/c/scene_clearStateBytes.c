#include "scene.h"
#include "variables.h"

/**
 * Zero the mission checkpoint variables.
 *
 * @romaddress 0x0800c6d0
 */
void scene_clearStateBytes(void)
{
    g_MissionCheckpointVars[0] = 0;
    g_MissionCheckpointVars[1] = 0;
    g_MissionCheckpointVars[2] = 0;
}
