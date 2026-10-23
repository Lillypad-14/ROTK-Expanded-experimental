#include "skill.h"
#include "player.h"
#include "variables.h"

/**
 * Spirit cost of an active skill.
 *
 * @romaddress 0x0804555c
 */
u16 skill_active_getSpiritCost(u8 playerIndex, u8 activeSkillIndex)
{
    return PLAYER_ACTIVE_SKILL(playerIndex, activeSkillIndex).spiritCost;
}
