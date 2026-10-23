#include "game.h"
#include "mission.h"
#include "scene.h"
#include "variables.h"

/**
 * Script op 44: set the mission spawn point and change scene.
 *
 * @romaddress 0x08018728
 */
void mission_script_op44(const u8 *command)
{
    u8 i;

    for (i = 0; i < 2; i++)
        g_MissionSpawnPoint[i] = command[5];
    game_requestSceneChangeWithArgs(SCENE_ID_UNUSED_15, command[6], command[4]);
    g_SceneNext.args[2] = 0;
    g_GameFlags.p |= GAME_FLAG_31;
}
