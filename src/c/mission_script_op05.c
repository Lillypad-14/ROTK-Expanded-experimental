#include "game.h"
#include "mission.h"
#include "scene.h"
#include "variables.h"

/**
 * Script op 5: show an in-game dialog, or in co-op spawn a group and start a stream.
 *
 * @romaddress 0x08018454
 */
void mission_script_op05(const u8 *command)
{
    scene_inGameDialog_resetState();
    g_InGameDialogState.resumeGroup = command[6];
    g_InGameDialogState.resumeStream = command[7];
    if (!(g_GameFlags.p & GAME_FLAG_COOP_ACTIVE))
        scene_requestInGameDialog(*(const u16 *)(command + 4));
    else
        mission_script_spawnGroupAndStartStream(g_InGameDialogState.resumeGroup, g_InGameDialogState.resumeStream);
}
