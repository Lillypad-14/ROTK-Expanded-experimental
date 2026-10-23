#include "mission.h"
#include "variables.h"

/**
 * Script op 60: subtract from a mission script variable.
 *
 * @romaddress 0x080189fc
 */
void mission_script_op60(const u8 *command)
{
    g_MissionScriptVars[command[4]] -= command[5];
}
