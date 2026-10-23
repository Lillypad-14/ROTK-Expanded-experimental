#include "actor.h"
#include "mission.h"

/**
 * Script op 37: wake a stunnable actor and reset it to its default behavior.
 *
 * @romaddress 0x080186ec
 */
void mission_script_op37(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor && actor_isStunnable(actor))
    {
        actor->flags.p |= 0x204;
        actor->flags.p &= ~ACTOR_FLAG_TICK_ALWAYS;
        actor_initBehaviorByType(actor);
    }
}
