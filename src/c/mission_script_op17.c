#include "actor.h"
#include "math.h"
#include "mission.h"

/**
 * Script op 17: set an actor's move speed and put it into behavior state 6.
 *
 * @romaddress 0x080176c0
 */
void mission_script_op17(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
    {
        actor->moveSpeed = math_signMagnitudeToFp16(*(const s16 *)(command + 6));
        actor->velocity.x = 0;
        actor->velocity.y = math_multiplyFp16(0x10000, 0);
        actor->recoveryCooldown = *(const u16 *)(command + 8);
        actor->flags.p &= ~(ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_ANIMATION_PLAYING);
        actor->field_0x73 = actor->behaviorState;
        actor->behaviorState = 6;
    }
}
