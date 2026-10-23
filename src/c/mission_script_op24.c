#include "actor.h"
#include "mission.h"

/**
 * Script op 24: suppress actor ticks and arm a countdown that starts a spawn group and stream.
 *
 * @romaddress 0x08017934
 */
void mission_script_op24(const u8 *command)
{
    Actor *timer;

    actor_setFlagsAll(ACTOR_FLAG_TICK_SUPPRESSED);
    timer = mission_script_allocTimer(0);
    timer->as.missionTimer.field_0x6 = *(const u16 *)(command + 4);
    timer->as.missionTimer.field_0x8 = *(const u16 *)(command + 6);
    timer->as.missionTimer.field_0xa = command[9];
    timer->as.missionTimer.field_0xb = command[10];
    timer->actionStateTimer = command[8];
    timer->behaviorState = 4;
    timer->as.missionTimer.groupIndex = command[11];
    timer->as.missionTimer.streamIndex = command[12];
}
