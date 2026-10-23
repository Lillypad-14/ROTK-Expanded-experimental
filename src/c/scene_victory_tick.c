#include "game.h"
#include "input.h"
#include "player.h"
#include "scene.h"
#include "scene/victory.h"
#include "variables.h"

/**
 * Leave to the save menu when a button is pressed, or when player 2 is present.
 *
 * @romaddress 0x08020a70
 */
void scene_victory_tick(void)
{
    if ((input_isPlayerPresent(0) && (PLAYER_KEYS_CURRENT(ACTIVE_PLAYER_INDEX) & 0x30f)) ||
        input_isPlayerPresent(1))
    {
        g_GameFlags.p |= 1 << 9;
        game_requestSceneChangeWithArgs(SCENE_ID_SAVE_LOAD_MENU, 3, 0);
    }
}
