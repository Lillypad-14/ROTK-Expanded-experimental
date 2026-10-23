#include "actor.h"
#include "mission.h"

/**
 * Script op 12: arm a countdown that starts a spawn group and stream when it expires.
 *
 * @romaddress 0x08018504
 */
void mission_script_op12(const u8 *command)
{
    Actor *timer = mission_script_allocTimer(1);

    timer->actionStateTimer = *(const u16 *)(command + 4);
    timer->as.missionTimer.groupIndex = command[6];
    timer->as.missionTimer.streamIndex = command[7];
}
