#include "player.h"
#include "scene/skillTree.h"
#include "variables.h"

/**
 * Draw the passive skill tiles of a player that unlock at the given level.
 *
 * @return the tile after the last one drawn
 *
 * @romaddress 0x080427c4
 */
u8 scene_skillTree_drawPassiveTilesAtLevel(u8 playerIndex, u8 tile, u8 level)
{
    u8 slot = 0;

    do
    {
        u8 skillId = PLAYER(playerIndex).passiveSkillIds[slot];

        if (PassiveSkillRequiredLevels[skillId][0] == level)
        {
            g_SkillTreePassiveTileSkillIds[tile] = skillId;
            scene_skillTree_drawPassiveTile(playerIndex, skillId, tile, PLAYER(playerIndex).passiveSkillLevelsApplied[slot]);
            tile++;
        }
        slot++;
    } while (slot <= 8);
    return tile;
}
