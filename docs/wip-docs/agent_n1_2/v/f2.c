#include "mission.h"
#include "player.h"
#include "variables.h"

void player_applyFrodoSamSwap(u32 playerIndex)
{
    Actor *actor = PLAYER(playerIndex).ownerActor;
    HeroId heroId = PLAYER(playerIndex).heroId;

    if (heroId == HERO_ID_FRODO && g_CurrentMissionId == MISSION_CIRITH_UNGOL_AMBUSH &&
        g_MissionVariant == 7)
    {
        PLAYER(playerIndex).heroId = HERO_ID_SAM;
        PLAYER(playerIndex).baseStats.health += 5;
        PLAYER(playerIndex).baseStats.strength += 5;
    }
    else
    {
        if (heroId != HERO_ID_SAM || g_CurrentMissionId != MISSION_CIRITH_UNGOL_CAMPSITE)
        {
            return;
        }
        PLAYER(playerIndex).heroId = HERO_ID_FRODO;
        PLAYER(playerIndex).baseStats.health -= 5;
        PLAYER(playerIndex).baseStats.strength -= 5;
    }
    actor->as.combat.venomDuration = 0;
    actor->as.combat.fearDuration = 0;
}
