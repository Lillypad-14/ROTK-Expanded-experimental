#include "actor.h"
#include "mission.h"

/**
 * Script op 51: park every actor of a spawn group in behavior state 1 and keep it ticking.
 *
 * @romaddress 0x080188b8
 */
void mission_script_op51(const u8 *command)
{
    u8 group = command[4];

    if (mission_script_slotNeedsLoad(group))
    {
        u16 count = mission_script_getGroupActorCount(group);
        u8 i;

        for (i = 0; i < count; i++)
        {
            Actor *actor = mission_script_getGroupActor(group, i);

            if (actor)
            {
                actor->behaviorState = 1;
                actor->flags.p |= ACTOR_FLAG_TICK_ALWAYS;
            }
        }
    }
}
