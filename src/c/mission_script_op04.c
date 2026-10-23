#include "actor.h"
#include "mission.h"

/**
 * Script op 4: clear an actor flag bit.
 *
 * @romaddress 0x0801842c
 */
void mission_script_op04(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
        actor->flags.p &= ~(1 << command[6]);
}
