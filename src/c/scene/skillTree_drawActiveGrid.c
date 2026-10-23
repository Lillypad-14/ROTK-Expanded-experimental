#include "player.h"
#include "scene/skillTree.h"

/**
 * Draw the active skill tiles for a player.
 *
 * @romaddress 0x08042854
 */
void scene_skillTree_drawActiveGrid(u8 playerIndex)
{
    scene_skillTree_drawActiveTiles(playerIndex, 0);
}
