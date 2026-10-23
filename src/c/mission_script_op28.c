#include "game.h"
#include "mission.h"
#include "multiplayer.h"
#include "scene.h"
#include "variables.h"

/**
 * Script op 28: set the mission spawn point and change to the cutscene scene.
 *
 * @romaddress 0x08018670
 */
void mission_script_op28(const u8 *command)
{
    u8 i;

    for (i = 0; i < 2; i++)
        g_MissionSpawnPoint[i] = command[5];
    game_requestSceneChangeWithArgs(SCENE_ID_CUTSCENE, 0, command[6]);
    g_SceneNext.args[2] = command[4];
    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) && g_SceneNext.args[1] == 58)
        multiplayer_endSession();
    if (g_GameFlags.p & GAME_FLAG_IN_GAME_DIALOG)
        g_GameFlags.p &= ~GAME_FLAG_IN_GAME_DIALOG;
}
