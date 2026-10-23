#include "player.h"
#include "scene/skillTree.h"

/**
 * Draw every passive skill tile row for a player.
 *
 * @romaddress 0x0804279c
 */
void scene_skillTree_drawPassiveGrid(u8 playerIndex)
{
    u8 tile = 0;
    u8 level;

    for (level = 0; level <= 20; level++)
    {
        tile = scene_skillTree_drawPassiveTilesAtLevel(playerIndex, tile, level);
    }
}
