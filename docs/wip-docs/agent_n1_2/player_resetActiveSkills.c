#include "game.h"
#include "player.h"
#include "variables.h"

/**
 * Reset the skill levels of both players: every passive and active level is 5 under
 * GAME_FLAG_2 and 0 otherwise, with Herbal Healing at level 1, then reset the skill controls.
 *
 * @romaddress 0x080428b0
 */
void player_resetActiveSkills(void)
{
    s32 playerIndex;

    for (playerIndex = 0; playerIndex <= 1; playerIndex++) {
        s8 *passive = PLAYER(playerIndex).passiveSkillLevelsPurchased;
        u8 *active;
        s32 i;

        for (i = HERO_PASSIVE_SKILL_COUNT - 1; i >= 0; i--) {
            u32 level = g_GameFlags.p & GAME_FLAG_2;

            if (level != 0) {
                level = 5;
            }
            passive[HERO_PASSIVE_SKILL_COUNT] = level;
            *passive = level;
            passive++;
        }
        active = PLAYER(playerIndex).activeSkillLevelsPurchased;
        for (i = 5; i >= 0; i--) {
            if (g_GameFlags.p & GAME_FLAG_2) {
                active[6] = 5;
                *active = 5;
            } else {
                active[6] = 0;
                *active = 0;
            }
            active++;
        }
        PLAYER(playerIndex).activeSkillLevelsPurchased[ACTIVE_SKILL_HERBAL_HEALING] = 1;
        PLAYER(playerIndex).activeSkillLevels[ACTIVE_SKILL_HERBAL_HEALING] = 1;
    }
    player_resetActiveSkillControls();
}
