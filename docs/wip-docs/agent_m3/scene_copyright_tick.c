#include "game.h"
#include "input.h"
#include "player.h"
#include "scene.h"
#include "variables.h"

u8 input_isPlayerPresent(u32 playerIndex);

/**
 * Skip to the intro logo when a button is pressed, or when player 2 is present.
 *
 * @romaddress 0x0802ff1c
 */
void scene_copyright_tick(void)
{
    if ((input_isPlayerPresent(0) && (PLAYER_KEYS_CURRENT(ACTIVE_PLAYER_INDEX) & 0x30f)) ||
        input_isPlayerPresent(1))
    {
        game_requestSceneChange(SCENE_ID_INTRO_LOGO);
    }
}
