#include "actor.h"
#include "mission.h"

/**
 * Script op 32: send a stunnable actor into behavior state 24 with a target position.
 *
 * @romaddress 0x08017c34
 */
void mission_script_op32(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[8], command[9]);

    if (actor && actor_isStunnable(actor))
    {
        actor->as.combat.head.scripted.targetX = *(const u16 *)(command + 4);
        actor->as.combat.head.scripted.targetY = *(const u16 *)(command + 6);
        actor->as.combat.head.enemyAiParams[4] = command[10];
        actor->behaviorState = 24;
        actor->field_0x64[0] |= 2;
        actor->actionState = (ActorActionState)0x12;
        actor->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TICK_ALWAYS | ACTOR_FLAG_TILE_COLLISION);
        actor->oam.bgPriority = 1;
    }
}
