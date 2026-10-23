#include "mission.h"
#include "variables.h"

/**
 * Script op 63: set a mission checkpoint variable.
 *
 * @romaddress 0x08018a30
 */
void mission_script_op63(const u8 *command)
{
    g_MissionCheckpointVars[command[4]] = command[5];
}
