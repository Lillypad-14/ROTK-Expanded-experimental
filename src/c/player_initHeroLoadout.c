#include "player.h"
#include "skill.h"
#include "variables.h"

/**
 * Point a player at the hero's skill tables and copy the purchased skill levels into the applied
 * and active level arrays, applying the passive bonuses in between.
 *
 * @romaddress 0x0804296c
 */
void player_initHeroLoadout(u8 heroId, u8 playerIndex)
{
    u8 i;

    switch (heroId)
    {
    case HERO_ID_SAM:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsSam;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsSam;
        PLAYER(playerIndex).activeSkills = ActiveSkillsSam;
        break;
    case HERO_ID_SMEAGOL:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsSmeagol;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsSmeagol;
        PLAYER(playerIndex).activeSkills = ActiveSkillsSmeagol;
        break;
    case HERO_ID_FRODO:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsFrodo;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconAssets[0];
        PLAYER(playerIndex).activeSkills = ActiveSkillsFrodo;
        break;
    case HERO_ID_LEGOLAS:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsLegolas;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsLegolas;
        PLAYER(playerIndex).activeSkills = ActiveSkillsLegolas;
        break;
    case HERO_ID_ARAGORN:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsAragorn;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsAragorn;
        PLAYER(playerIndex).activeSkills = ActiveSkillsAragorn;
        break;
    case HERO_ID_GANDALF:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsGandalf;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsGandalf;
        PLAYER(playerIndex).activeSkills = ActiveSkillsGandalf;
        break;
    case HERO_ID_EOWYN:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsEowyn;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsEowyn;
        PLAYER(playerIndex).activeSkills = ActiveSkillsEowyn;
        break;
    case HERO_ID_GIMLI:
        PLAYER(playerIndex).passiveSkillIds = PassiveSkillIdsGimli;
        PLAYER(playerIndex).activeSkillIcons = ActiveSkillIconsGimli;
        PLAYER(playerIndex).activeSkills = ActiveSkillsGimli;
        break;
    }
    for (i = 0; i <= 8; i++)
        PLAYER(playerIndex).passiveSkillLevelsApplied[i] =
            PLAYER(playerIndex).passiveSkillLevelsPurchased[i];
    skill_passive_applyAllLevels(playerIndex);
    for (i = 0; i <= 5; i++)
        PLAYER(playerIndex).activeSkillLevels[i] =
            PLAYER(playerIndex).activeSkillLevelsPurchased[i];
}
