#include "game.h"
#include "input.h"
#include "player.h"
#include "scene.h"
#include "scene/newGame.h"
#include "sfx.h"
#include "sprite.h"
#include "variables.h"

void scene_newGame_unknown080142a4(u8 arg);
void scene_newGame_unknown08014338(void);
void scene_newGame_unknown080144f4(void);

// Stand-in for the byte at 0x03001ab4.
#define g_NewGameState (*(u8 *)0x03001ab4)

/**
 * Per-frame body of the new game intro: advance on A/Start, switch on the d-pad, back out on B.
 *
 * @romaddress 0x08014230
 */
void scene_newGame_tick(void)
{
    if (g_NewGameState != 0)
    {
        if (PLAYER_KEYS_PRESSED(ACTIVE_PLAYER_INDEX) & 9)
        {
            scene_newGame_unknown080142a4(g_SceneCurrent.args[1]);
        }
        else if (PLAYER_KEYS_PRESSED(ACTIVE_PLAYER_INDEX) & 0xf0)
        {
            scene_newGame_unknown080144f4();
        }
        else if (PLAYER_KEYS_PRESSED(ACTIVE_PLAYER_INDEX) & 6)
        {
            game_requestSceneChange(SCENE_ID_MAIN_MENU);
            sfx_play(4);
        }
    }
    else
    {
        scene_newGame_unknown08014338();
    }
    actor_tickAll();
    sprite_endFrame();
}
