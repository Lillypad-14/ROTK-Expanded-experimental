#include "mission.h"
#include "variables.h"

/**
 * Script op 58: set a mission script variable.
 *
 * @romaddress 0x080189d8
 */
void mission_script_op58(const u8 *command)
{
    g_MissionScriptVars[command[4]] = command[5];
}
