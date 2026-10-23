#include "game.h"
#include "mission.h"
#include "variables.h"

/**
 * Script op 68: spawn a group and start a stream, using the second pair of ids unless co-op
 * is active.
 *
 * @romaddress 0x08018a94
 */
void mission_script_op68(const u8 *command)
{
    if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE)
        mission_script_spawnGroupAndStartStream(command[4], command[5]);
    else
        mission_script_spawnGroupAndStartStream(command[6], command[7]);
}
