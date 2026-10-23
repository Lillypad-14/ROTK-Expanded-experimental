#include "player.h"
#include "scene/skillTree.h"
#include "variables.h"

/**
 * Draw the six active skill tiles of a player starting at a tile.
 *
 * @return the tile after the last one drawn
 *
 * @romaddress 0x08042864
 */
u8 scene_skillTree_drawActiveTiles(u8 playerIndex, u32 startTile)
{
    u8 tile = startTile;
    u8 row = 0;

    do
    {
        scene_skillTree_drawActiveTile(playerIndex, row, tile, PLAYER(playerIndex).activeSkillLevels[row]);
        tile++;
        row++;
    } while (row <= 5);
    return tile;
}
