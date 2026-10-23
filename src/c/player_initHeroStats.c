#include "player.h"
#include "skill.h"
#include "variables.h"

/**
 * Write the hero's starting primary stats into baseStats and reset the active skill selection.
 *
 * @romaddress 0x08025d6c
 */
void player_initHeroStats(u8 heroId, u8 playerIndex)
{
    switch (heroId)
    {
    case HERO_ID_FRODO:
        PLAYER(playerIndex).baseStats.courage = 30;
        PLAYER(playerIndex).baseStats.defense = 28;
        PLAYER(playerIndex).baseStats.health = 18;
        PLAYER(playerIndex).baseStats.accuracy = 21;
        PLAYER(playerIndex).baseStats.strength = 16;
        break;
    case HERO_ID_LEGOLAS:
        PLAYER(playerIndex).baseStats.courage = 20;
        PLAYER(playerIndex).baseStats.defense = 22;
        PLAYER(playerIndex).baseStats.health = 20;
        PLAYER(playerIndex).baseStats.accuracy = 30;
        PLAYER(playerIndex).baseStats.strength = 18;
        break;
    case HERO_ID_ARAGORN:
        PLAYER(playerIndex).baseStats.courage = 20;
        PLAYER(playerIndex).baseStats.defense = 24;
        PLAYER(playerIndex).baseStats.health = 22;
        PLAYER(playerIndex).baseStats.accuracy = 23;
        PLAYER(playerIndex).baseStats.strength = 23;
        break;
    case HERO_ID_GANDALF:
        PLAYER(playerIndex).baseStats.courage = 30;
        PLAYER(playerIndex).baseStats.defense = 20;
        PLAYER(playerIndex).baseStats.health = 22;
        PLAYER(playerIndex).baseStats.accuracy = 20;
        PLAYER(playerIndex).baseStats.strength = 18;
        break;
    case HERO_ID_EOWYN:
        PLAYER(playerIndex).baseStats.courage = 25;
        PLAYER(playerIndex).baseStats.defense = 20;
        PLAYER(playerIndex).baseStats.health = 24;
        PLAYER(playerIndex).baseStats.accuracy = 25;
        PLAYER(playerIndex).baseStats.strength = 18;
        break;
    case HERO_ID_GIMLI:
        PLAYER(playerIndex).baseStats.courage = 20;
        PLAYER(playerIndex).baseStats.defense = 21;
        PLAYER(playerIndex).baseStats.health = 21;
        PLAYER(playerIndex).baseStats.accuracy = 22;
        PLAYER(playerIndex).baseStats.strength = 28;
        break;
    case HERO_ID_SAM:
        PLAYER(playerIndex).baseStats.courage = 34;
        PLAYER(playerIndex).baseStats.defense = 25;
        PLAYER(playerIndex).baseStats.health = 22;
        PLAYER(playerIndex).baseStats.accuracy = 18;
        PLAYER(playerIndex).baseStats.strength = 17;
        break;
    case HERO_ID_SMEAGOL:
        PLAYER(playerIndex).baseStats.courage = 12;
        PLAYER(playerIndex).baseStats.defense = 22;
        PLAYER(playerIndex).baseStats.health = 25;
        PLAYER(playerIndex).baseStats.accuracy = 30;
        PLAYER(playerIndex).baseStats.strength = 29;
    }
    player_recomputeBaseStats(playerIndex);
    PLAYER(playerIndex).selectedActiveSkill.cycleSkillIndex = ACTIVE_SKILL_NONE;
    PLAYER(playerIndex).whetstoneLevel = 15;
}
