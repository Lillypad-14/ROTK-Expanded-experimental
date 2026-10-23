#include "game.h"
#include "mission.h"
#include "player.h"
#include "scene.h"
#include "scene/newGame.h"
#include "sfx.h"
#include "variables.h"

/**
 * Start a new game as a hero: set up the first player and request the hero's intro cutscene.
 *
 * @romaddress 0x080142a4
 */
void scene_newGame_unknown080142a4(u8 heroId)
{
    PLAYER(0).heroId = heroId;
    if (heroId <= 5)
    {
        game_requestSceneChangeWithArgs(SCENE_ID_CUTSCENE, 0, HeroStartCutscenes[heroId]);
        g_SceneNext.args[2] = HeroStartMissions[heroId];
        g_MissionVariant = HeroStartVariants[heroId];
    }
    g_PlayerGlobals.activePlayerCount = 1;
    g_MissionSpawnPoint[0] = 0;
    player_initHeroStats(PLAYER(0).heroId, 0);
    player_initHeroLoadout(PLAYER(0).heroId, 0);
    player_initItemsAndHollow(0);
    player_applyOptionFlags(g_PlayerGlobals.activePlayerIndex);
    sfx_play(1);
}
