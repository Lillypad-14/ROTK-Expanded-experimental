#include "player.h"
#include "scene/skillTree.h"
#include "sfx.h"
#include "variables.h"

/**
 * Toggle whether an active skill row takes part in the cast cycle and redraw its tile.
 *
 * @romaddress 0x0804269c
 */
void scene_skillTree_unknown0804269c(s32 playerIndex, s32 row)
{
    if (PLAYER(playerIndex).activeSkillCycleEnabled[row] == 0)
    {
        PLAYER(playerIndex).activeSkillCycleEnabled[row] = 1;
    }
    else
    {
        PLAYER(playerIndex).activeSkillCycleEnabled[row] = 0;
    }
    if (playerIndex == ACTIVE_PLAYER_INDEX)
    {
        sfx_play(3);
        scene_skillTree_drawActiveTile(playerIndex, row, row, PLAYER(playerIndex).activeSkillLevelsPurchased[row]);
    }
}
