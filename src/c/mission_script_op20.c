#include "mission.h"
/**
 * Script op 20: look up a group actor and discard it.
 *
 * @romaddress 0x080185bc
 */
void mission_script_op20(const u8 *command)
{
    mission_script_getGroupActor(command[4], command[5]);
}
