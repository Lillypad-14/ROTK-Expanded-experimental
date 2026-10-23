#include "mission.h"
#include "variables.h"

/**
 * Script op 62: compare a checkpoint variable against a value and start one of two spawn group
 * and stream pairs depending on the result.
 *
 * @romaddress 0x08018308
 */
void mission_script_op62(const u8 *command)
{
    u8 value = g_MissionCheckpointVars[command[4]];
    u8 op = command[5];
    u8 operand = command[6];
    u8 passGroup = command[7];
    u8 passStream = command[8];
    u8 failGroup = command[9];
    u8 failStream = command[10];
    u8 passed;

    switch (op)
    {
    case 0:
        passed = (value < operand) ? 1 : 0;
        break;
    case 1:
        passed = (value <= operand) ? 1 : 0;
        break;
    case 2:
        passed = (value == operand) ? 1 : 0;
        break;
    case 5:
        passed = value != operand;
        break;
    case 3:
        passed = (value >= operand) ? 1 : 0;
        break;
    case 4:
        passed = (value > operand) ? 1 : 0;
        break;
    default:
        passed = 0;
        break;
    }
    if (passed)
        mission_script_spawnGroupAndStartStream(passGroup, passStream);
    else
        mission_script_spawnGroupAndStartStream(failGroup, failStream);
}
