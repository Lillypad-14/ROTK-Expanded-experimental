#include "game.h"
#include "mission.h"
#include "variables.h"

/**
 * Script op 47: return to the main menu and set the pending scene change flag.
 *
 * @romaddress 0x08018798
 */
void mission_script_op47(const u8 *command)
{
    (void)command;
    game_requestSceneChange(SCENE_ID_MAIN_MENU);
    g_GameFlags.p |= GAME_FLAG_31;
}
