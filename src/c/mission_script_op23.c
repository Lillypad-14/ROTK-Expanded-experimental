#include "actor.h"
#include "mission.h"

/**
 * Script op 23: place an actor at a position and facing.
 *
 * @romaddress 0x08018624
 */
void mission_script_op23(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[8], command[9]);

    if (actor)
    {
        actor->coords.x = *(const s16 *)(command + 4) << 16;
        actor->coords.y = *(const s16 *)(command + 6) << 16;
        actor->facing = command[10] >> 1;
        actor->field_0x64[0] |= 1;
    }
}
