#include "actor.h"
#include "mission.h"

/**
 * Script op 49: clear an actor flag bit on every actor of a spawn group.
 *
 * @romaddress 0x08018804
 */
void mission_script_op49(const u8 *command)
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
                actor->flags.p &= ~(1 << command[5]);
            }
        }
    }
}
