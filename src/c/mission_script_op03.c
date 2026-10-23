#include "actor.h"
#include "mission.h"

/**
 * Script op 3: set an actor flag bit.
 *
 * @romaddress 0x08018404
 */
void mission_script_op03(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
        actor->flags.p |= 1 << command[6];
}
