#include "actor.h"
#include "mission.h"

/**
 * Script op 50: wake every stunnable actor of a spawn group and reset it to its default behavior.
 *
 * @romaddress 0x08018854
 */
void mission_script_op50(const u8 *command)
{
    u8 group = command[4];

    if (mission_script_slotNeedsLoad(group))
    {
        u16 count = mission_script_getGroupActorCount(group);
        u8 i;

        for (i = 0; i < count; i++)
        {
            Actor *actor = mission_script_getGroupActor(group, i);

            if (actor && actor_isStunnable(actor))
            {
                actor->flags.p |= 0x204;
                actor->flags.p &= ~ACTOR_FLAG_TICK_ALWAYS;
                actor_initBehaviorByType(actor);
            }
        }
    }
}
