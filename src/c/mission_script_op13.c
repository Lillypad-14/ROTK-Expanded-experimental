#include "actor.h"
#include "mission.h"

/**
 * Script op 13: decrement the use counter of a gate panel or switch, clamped at 0.
 *
 * @romaddress 0x08017474
 */
void mission_script_op13(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);
    s8 *counter;

    if (actor)
    {
        if (actor->type == ACTOR_TYPE_GATE_PANEL)
            counter = (s8 *)&actor->as.gatePanel.useCounter;
        else if (actor->type == ACTOR_TYPE_SWITCH)
            counter = (s8 *)&actor->as.switch_.useCounter;
        else
            return;
        if (--*counter < 0)
            *counter = 0;
    }
}
