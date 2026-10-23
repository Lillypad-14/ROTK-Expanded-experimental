#include "actor.h"
#include "mission.h"

/**
 * Script op 14: raise the use counter of a gate panel or switch.
 *
 * @romaddress 0x08018528
 */
void mission_script_op14(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);
    u8 *counter;

    if (actor)
    {
        if (actor->type == ACTOR_TYPE_GATE_PANEL)
            counter = (u8 *)&actor->as.gatePanel.useCounter;
        else if (actor->type == ACTOR_TYPE_SWITCH)
            counter = (u8 *)&actor->as.switch_.useCounter;
        else
            return;
        *counter += 1;
    }
}
