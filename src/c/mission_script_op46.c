#include "game.h"
#include "mission.h"
#include "multiplayer.h"
#include "variables.h"

/**
 * Script op 46: end the multiplayer session and request a scene change.
 *
 * @romaddress 0x08018770
 */
void mission_script_op46(const u8 *command)
{
    (void)command;
    game_requestSceneChangeWithArgs(SCENE_ID_SAVE_LOAD_MENU, 3, 0);
    multiplayer_endSession();
    g_GameFlags.p |= 0x80000200;
}
