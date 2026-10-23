#include "actor.h"
#include "mission.h"

/**
 * Script op 41: send a stunnable actor into behavior state 36 with a parameter byte.
 *
 * @romaddress 0x08017dac
 */
void mission_script_op41(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor && actor_isStunnable(actor))
    {
        actor->as.combat.head.enemyAiParams[4] = *(const u16 *)(command + 6);
        actor->behaviorState = 36;
        actor->actionStateTimer = 0;
        actor->actionState = (ActorActionState)0x12;
        actor->field_0x64[0] |= 2;
        actor->flags.p |= 0x204;
        actor->flags.p &= ~ACTOR_FLAG_TICK_ALWAYS;
    }
}
