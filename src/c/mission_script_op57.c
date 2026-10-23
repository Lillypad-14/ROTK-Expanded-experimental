#include "mission.h"
#include "variables.h"

/**
 * Script op 57: compare a script variable against a value and start one of two spawn group and
 * stream pairs depending on the result.
 *
 * @romaddress 0x080189a8
 */
void mission_script_op57(const u8 *command)
{
    mission_script_compareAndSpawn(g_MissionScriptVars[command[4]], command[6], command[5], command[7], command[8],
                                   command[9], command[10]);
}
