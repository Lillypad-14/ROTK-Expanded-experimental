#include "mission.h"
#include "variables.h"

/**
 * Script op 64: add to a mission checkpoint variable.
 *
 * @romaddress 0x08018a40
 */
void mission_script_op64(const u8 *command)
{
    g_MissionCheckpointVars[command[4]] += command[5];
}
