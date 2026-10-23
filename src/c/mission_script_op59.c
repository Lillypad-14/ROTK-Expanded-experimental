#include "mission.h"
#include "variables.h"

/**
 * Script op 59: add to a mission script variable.
 *
 * @romaddress 0x080189e8
 */
void mission_script_op59(const u8 *command)
{
    g_MissionScriptVars[command[4]] += command[5];
}
