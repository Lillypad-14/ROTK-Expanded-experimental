#include "actor.h"
#include "mission.h"

/**
 * Script op 33: send a stunnable actor into behavior state 14 with a target position and
 * parameters.
 *
 * @romaddress 0x080173f0
 */
void mission_script_op33(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[8], command[9]);

    if (actor && actor_isStunnable(actor))
    {
        actor->as.combat.head.scripted.targetX = *(const u16 *)(command + 4);
        actor->as.combat.head.scripted.targetY = *(const u16 *)(command + 6);
        actor->as.combat.head.scripted.params[0] = command[12];
        actor->as.combat.head.scripted.params[1] = command[10];
        actor->as.combat.head.scripted.params[2] = command[11];
        actor->behaviorState = 14;
        actor->actionState = (ActorActionState)0x12;
        if (actor->type == 0)
            actor_player_initScripted(actor);
        else
            actor->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
        actor->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION);
    }
}
