#include "game.h"
#include "mission.h"
#include "variables.h"

/**
 * Script op 19: set the mission spawn point and restart gameplay on a mission id.
 *
 * @romaddress 0x08018580
 */
void mission_script_op19(const u8 *command)
{
    u8 i;

    for (i = 0; i < 2; i++)
        g_MissionSpawnPoint[i] = command[5];
    game_requestSceneChangeWithArgs(SCENE_ID_GAMEPLAY, 0, command[4]);
    g_GameFlags.p |= GAME_FLAG_31;
}
