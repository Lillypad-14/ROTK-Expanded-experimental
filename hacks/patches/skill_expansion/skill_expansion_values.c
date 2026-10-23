#include "player.h"
#include "skill.h"
#include "stats.h"
#include "variables.h"

/**
 * Cap curve reads while retaining temporary levels in the player's level counters.
 *
 * REPLACES (whole function): skill_active_getLeveledValue.
 */
s16 skillExpansion_activeGetLeveledValue(u8 playerIndex, u8 activeSkillIndex, u8 valueIndex,
                                         u8 levelCount)
{
    const ActiveSkillValue *value =
        &PLAYER_ACTIVE_SKILL(playerIndex, activeSkillIndex).values[valueIndex];
    u16 total = value->base;
    u32 level;

    if (levelCount > ACTIVE_SKILL_MAX_LEVEL)
    {
        levelCount = ACTIVE_SKILL_MAX_LEVEL;
    }
    for (level = 0; level < levelCount; level++)
    {
        total += value->perLevel[level];
    }
    if (PLAYER(playerIndex).stats[STAT_ACTIVE_SKILL_LEVEL_BONUS] > 0)
    {
        total +=
            value->perLevel[level < ACTIVE_SKILL_MAX_LEVEL ? level : ACTIVE_SKILL_MAX_LEVEL - 1];
    }
    return total;
}
