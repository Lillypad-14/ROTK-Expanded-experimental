#include "actor.h"
#include "mission.h"

/**
 * Script op 1: park an actor in behavior state 1 and keep it ticking.
 *
 * @romaddress 0x080183a8
 */
void mission_script_op01(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
    {
        actor->behaviorState = 1;
        actor->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
    }
}
