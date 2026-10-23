#include "mission.h"
#include "rng.h"
#include "variables.h"

/**
 * Script op 61: store a random value in a mission script variable.
 *
 * @romaddress 0x08018a10
 */
void mission_script_op61(const u8 *command)
{
    g_MissionScriptVars[command[4]] = rng_rollRange(command[5], command[6]);
}
