#include "game.h"
#include "input.h"
#include "player.h"
#include "scene.h"
#include "scene/inGameDialog.h"
#include "variables.h"

/**
 * Advance the dialog on A and return to the previous scene once it has finished.
 *
 * @romaddress 0x0801ad34
 */
void scene_inGameDialog_tick(void)
{
    if ((g_PlayerKeysPressed[ACTIVE_PLAYER_INDEX] & 1) && g_InGameDialogState.field_0x6 <= 1)
    {
        scene_inGameDialog_unknown0801ac78();
    }
    else if (g_InGameDialogState.field_0x6 == 7)
    {
        g_GameFlags.p &= ~GAME_FLAG_IN_GAME_DIALOG;
        game_requestSceneChangeWithArgs(g_ScenePrevious.id, 0, g_CurrentMissionId);
    }
}
