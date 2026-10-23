#include "actor.h"
#include "math.h"
#include "mission.h"

/**
 * Script op 34: send a stunnable actor into behavior state 22 with a target position.
 *
 * @romaddress 0x08017b3c
 */
void mission_script_op34(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);
    u8 *p;
    u8 v;

    if (actor && actor_isStunnable(actor))
    {
        actor->behaviorState = 22;
        actor->actionStateTimer = 0;
        actor->actionState = (ActorActionState)0x12;
        actor->field_0x64[0] = 0;
        actor->as.combat.head.scripted.targetY = *(const u16 *)(command + 6);
        actor->as.combat.head.scripted.targetX = *(const u16 *)(command + 8);
        v = command[10];
        p = &actor->as.combat.field_0x8c[1];
        *p = v;
        v = command[11];
        p++;
        *p = v;
        math_vector2_copy(&actor->coords, (Vector2Fp16 *)(p + 22));
        actor->flags.p |= 0x20c;
    }
}
