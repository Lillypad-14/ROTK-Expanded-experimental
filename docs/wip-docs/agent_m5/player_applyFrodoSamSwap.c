#include "mission.h"
#include "player.h"
#include "variables.h"

/**
 * Swap the player between Frodo and Sam for the Cirith Ungol ambush (Frodo becomes Sam) and
 * campsite (Sam becomes Frodo), moving 5 health and 5 strength, and clear the actor's poison and
 * fear.
 *
 * @romaddress 0x08025740
 */
void player_applyFrodoSamSwap(u32 playerIndex)
{
    Player *player = g_PlayerGlobals.players + playerIndex;
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
        player = g_PlayerGlobals.players + playerIndex;
        player->heroId = HERO_ID_FRODO;
        player->baseStats.health = player->baseStats.health - 5;
        strength = player->baseStats.strength - 5;
    }
    player->baseStats.strength = strength;
    actor->as.combat.venomDuration = 0;
    actor->as.combat.fearDuration = 0;
}
