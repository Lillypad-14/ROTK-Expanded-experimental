#include "skill.h"
#include "player.h"
#include "variables.h"

/**
 * Check whether the player has any level in an active skill.
 *
 * @romaddress 0x080454c0
 */
bool32 skill_active_hasLevels(u8 playerIndex, u8 activeSkillIndex)
{
    s8 level = PLAYER(playerIndex).activeSkillLevels[activeSkillIndex];

    if (level == 0)
    {
        return FALSE;
    }
    return TRUE;
}
