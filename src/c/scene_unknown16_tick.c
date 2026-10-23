#include "game.h"
#include "input.h"
#include "player.h"
#include "scene.h"
#include "scene/unknown16.h"
#include "sprite.h"
#include "variables.h"

/**
 * Skip to the intro logo on a button press or when player 2 is present.
 *
 * @romaddress 0x08030484
 */
void scene_unknown16_tick(void)
{
    if ((input_isPlayerPresent(0) && (PLAYER_KEYS_CURRENT(ACTIVE_PLAYER_INDEX) & 0x30f)) ||
        input_isPlayerPresent(1))
    {
        game_requestSceneChange(SCENE_ID_INTRO_LOGO);
    }
    sprite_endFrame();
}
