#include "player.h"
#include "scene/skillTree.h"
#include "variables.h"

/** @romaddress 0x08042674 */
void scene_skillTree_unknown08042674(void)
{
}

/** @romaddress 0x08042678 */
void scene_skillTree_unknown08042678(void)
{
}

/**
 * Take one skill point from the player after a purchase.
 *
 * @romaddress 0x0804267c
 */
void scene_skillTree_spendSkillPoint(u8 playerIndex)
{
    PLAYER(playerIndex).skillPoints--;
}
