#include "mission.h"
#include "variables.h"

/**
 * Script op 65: subtract from a mission checkpoint variable.
 *
 * @romaddress 0x08018a54
 */
void mission_script_op65(const u8 *command)
{
    g_MissionCheckpointVars[command[4]] -= command[5];
}
