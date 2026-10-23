#include "actor.h"
#include "math.h"
#include "mission.h"

/**
 * Script op 2: set an actor's move speed.
 *
 * @romaddress 0x080183d0
 */
void mission_script_op02(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
    {
        actor->moveSpeed = math_signMagnitudeToFp16(*(const s16 *)(command + 6));
        actor->field_0x64[0] |= 4;
    }
}
