#include "actor.h"
#include "player.h"
#include "variables.h"

/**
 * Reset the actor's action state and install actor_player_tickScripted as its tick. Sets bit 7
 * of the player's combat flags and records the player slot in unitType.
 *
 * @romaddress 0x080272d8
 */
void actor_player_initScripted(Actor *actor)
{
    actor_resetActionState(actor);
    actor->tickCallback = actor_player_tickScripted;
    PLAYER(actor->as.combat.playerIndex).combatFlags.p |= 1 << 7;
    actor->unitType = actor->as.combat.playerIndex;
}
