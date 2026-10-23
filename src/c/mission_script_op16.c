#include "actor.h"
#include "actor/player.h"
#include "mission.h"
#include "player.h"
#include "variables.h"

/**
 * Script op 16: put an actor into behavior state 4. A gate panel is marked activated while
 * the current stream's slot is loaded, and Frodo is switched to the scripted tick.
 *
 * @romaddress 0x080174ac
 */
void mission_script_op16(const u8 *command)
{
    Actor *actor = mission_script_getGroupActor(command[4], command[5]);

    if (actor)
    {
        actor->behaviorState = 4;
        actor->field_0x64[0] |= 0x10;
        if (actor->type == ACTOR_TYPE_GATE_PANEL && !mission_script_slotNeedsLoad(g_MissionCurrentStream))
        {
            actor->as.gatePanel.activated = 1;
            return;
        }
        if (actor->type == ACTOR_TYPE_UNKNOWN_00 && PLAYER(actor->as.combat.playerIndex).heroId == 0)
            actor_player_initScripted(actor);
    }
}
