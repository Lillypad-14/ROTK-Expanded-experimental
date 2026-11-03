// Bonus maps: picking an empty save slot opens the hero select screen (hero_select.c) in place of
// the save menu's Left/Right hero select (PC edition).
#include "patches/heroes/hero_grid.h"
#include "patches/heroes/hero_select.h"
#include "game.h"
#include "scene.h"
#include "scene/saveLoadMenu.h"
#include "variables.h"

bool32 heroGrid_active(void)
{
#if PC_EDITION && HACK_gauntlet && HACK_ringwraith
    return SCENE_ARG_SAVE_LOAD_MODE == SAVE_LOAD_MODE_BONUS_LOAD &&
           SCENE_ARG_SAVE_LOAD_STATE == SAVE_LOAD_STATE_HERO_SELECT && !heroSelect_committing();
#else
    return FALSE;
#endif
}

bool32 heroGrid_drawSlot(u8 slot)
{
    (void)slot;
    if (!heroGrid_active())
    {
        return FALSE;
    }
#if PC_EDITION && HACK_gauntlet && HACK_ringwraith
    if (!game_isSceneChangePending())
    {
        heroSelect_open();
    }
#endif
    return TRUE;
}
