#include "match_hacks.h"
#include "mission.h"

/**
 * Compare value with operand using comparison op (0 <, 1 <=, 2 ==, 3 >=, 4 >, 5 !=) and start
 * the pass or fail spawn group and stream pair.
 *
 * @romaddress 0x08018b18
 */
void mission_script_compareAndSpawn(u32 value, u32 operand, u32 op, u32 passGroup, u32 passStream, u32 failGroup,
                                    u32 failStream)
{
    u32 passed;

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
    }
    MATCH_BARRIER(passed);
    if (passed)
        mission_script_spawnGroupAndStartStream(passGroup, passStream);
    else
        mission_script_spawnGroupAndStartStream(failGroup, failStream);
}
