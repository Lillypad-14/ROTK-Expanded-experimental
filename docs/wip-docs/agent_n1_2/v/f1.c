#include "mission.h"
#include "player.h"
#include "variables.h"

void player_applyFrodoSamSwap(u32 playerIndex)
{
    Player *players = g_PlayerGlobals.players;
    Player *player = players + playerIndex;
    Actor *actor = player->ownerActor;
    HeroId *heroIdPtr = &player->heroId;
    HeroId heroId = *heroIdPtr;
    s32 strength;

    if (heroId == HERO_ID_FRODO && g_CurrentMissionId == MISSION_CIRITH_UNGOL_AMBUSH &&
        g_MissionVariant == 7)
    {
        *heroIdPtr = HERO_ID_SAM;
        player->baseStats.health = player->baseStats.health + 5;
        strength = player->baseStats.strength + 5;
    }
    else
    {
        if (heroId != HERO_ID_SAM || g_CurrentMissionId != MISSION_CIRITH_UNGOL_CAMPSITE)
        {
            return;
        }
        player = players + playerIndex;
        player->heroId = HERO_ID_FRODO;
        player->baseStats.health = player->baseStats.health - 5;
        strength = player->baseStats.strength - 5;
    }
    player->baseStats.strength = strength;
    actor->as.combat.venomDuration = 0;
    actor->as.combat.fearDuration = 0;
}
