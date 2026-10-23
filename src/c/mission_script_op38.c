#include "actor.h"
#include "mission.h"

/**
 * Script op 38: send a stunnable actor into behavior state 26.
 *
 * @romaddress 0x08017ca8
 */
void mission_script_op38(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor && actor_isStunnable(actor))
    {
        actor->behaviorState = 26;
        actor->actionStateTimer = 0;
        actor->actionState = (ActorActionState)0x12;
        actor->field_0x64[0] |= 2;
        actor->flags.p |= 0x204;
        actor->flags.p &= ~ACTOR_FLAG_TICK_ALWAYS;
    }
}
