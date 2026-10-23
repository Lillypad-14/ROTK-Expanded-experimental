#include "player.h"
#include "scene/skillTree.h"
#include "variables.h"

/**
 * A button on the skill tree: buy the selected skill for the active player.
 *
 * @romaddress 0x08042780
 */
void scene_skillTree_handleA(void)
{
    scene_skillTree_buySkill(ACTIVE_PLAYER_INDEX);
}
