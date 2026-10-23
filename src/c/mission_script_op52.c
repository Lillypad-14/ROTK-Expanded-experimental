#include "mission.h"
#include "variables.h"

/**
 * Script op 52: compare a script variable against a value and start one of two spawn group and
 * stream pairs depending on the result.
 *
 * @romaddress 0x0801820c
 */
void mission_script_op52(const u8 *command)
{
    u8 passed = 0;
    const u8 *var = MissionScriptVarPointers[command[4]];

    switch (command[5])
    {
    case 0:
        if (*var < command[6])
            passed = 1;
        break;
    case 1:
        if (*var <= command[6])
            passed = 1;
        break;
    case 2:
        if (*var == command[6])
            passed = 1;
        break;
    case 3:
        if (*var >= command[6])
            passed = 1;
        break;
    case 4:
        if (*var > command[6])
            passed = 1;
        break;
    }
    if (passed)
        mission_script_spawnGroupAndStartStream(command[7], command[8]);
    else
        mission_script_spawnGroupAndStartStream(command[9], command[10]);
}
