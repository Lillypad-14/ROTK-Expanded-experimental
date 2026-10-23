#include "actor.h"
#include "mission.h"

/**
 * Script op 55: send an actor into behavior state 41 with a target position.
 *
 * @romaddress 0x08018298
 */
void mission_script_op55(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[8], command[9]);

    if (actor)
    {
        actor->behaviorState = 41;
        actor->actionStateTimer = 0;
        actor->actionState = (ActorActionState)0x12;
        actor->field_0x64[0] |= 2;
        actor->as.combat.head.enemyAiParams[5] = command[10];
        actor->as.combat.head.enemyAiParams[6] = command[11];
        actor->as.combat.head.scripted.targetX = *(const u16 *)(command + 4);
        actor->as.combat.head.scripted.targetY = *(const u16 *)(command + 6);
        actor->moveSpeed = 0x58000;
        actor->flags.p &= ~0x204;
        actor->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
    }
}
