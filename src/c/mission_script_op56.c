#include "game.h"
#include "mission.h"
#include "variables.h"

/**
 * Script op 56: in co-op, set the mission spawn point and restart gameplay on a mission id.
 *
 * @romaddress 0x08018960
 */
void mission_script_op56(const u8 *command)
{
    u8 i;

    if (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE)
    {
        for (i = 0; i < 2; i++)
            g_MissionSpawnPoint[i] = command[5];
        game_requestSceneChangeWithArgs(SCENE_ID_GAMEPLAY, 0, command[4]);
        g_GameFlags.p |= GAME_FLAG_31;
    }
}
