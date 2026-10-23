#include "mission.h"
#include "rng.h"
#include "variables.h"

/**
 * Script op 66: store a random value in a mission checkpoint variable.
 *
 * @romaddress 0x08018a68
 */
void mission_script_op66(const u8 *command)
{
    g_MissionCheckpointVars[command[4]] = rng_rollRange(command[5], command[6]);
}
