#include "actor.h"
#include "mission.h"

/**
 * Script op 39: send a stunnable actor into behavior state 33 with a parameter byte.
 *
 * @romaddress 0x08017cfc
 */
void mission_script_op39(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor && actor_isStunnable(actor))
    {
        actor->as.combat.field_0x8c[0] = command[6];
        actor->behaviorState = 33;
        actor->actionStateTimer = 0;
        actor->actionState = (ActorActionState)0x12;
        actor->field_0x64[0] |= 2;
        actor->flags.p |= 0x204;
        actor->flags.p &= ~ACTOR_FLAG_TICK_ALWAYS;
    }
}
