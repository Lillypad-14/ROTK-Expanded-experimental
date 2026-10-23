#include "actor.h"
#include "match_hacks.h"
#include "player.h"
#include "variables.h"

/**
 * Set the actor's tick callback, move animation bank, playerIndex, and combat defaults right
 * after allocation.
 *
 * @romaddress 0x08025c2c
 */
void actor_player_initState(Actor *actor, u8 playerIndex)
{
    void *bank;
    u8 hero;

    actor->tickCallback = actor_player_tick;
    actor->as.combat.appliedPose = 4;
    actor->as.combat.requestedPose = 4;
    hero = PLAYER(playerIndex).heroId;
    actor->as.combat.head.player.animationBank = bank = (void *)HeroMoveAnimationBanks[hero];
    MATCH_BARRIER(actor);
    actor->as.combat.head.player.appliedAnimationBank = bank;
    actor->as.combat.playerIndex = playerIndex;
    actor->as.combat.field_0x97[1] = 0;
    actor->as.combat.comboState[3] = 0;
    actor->as.combat.comboState[1] = 30;
    PLAYER(playerIndex).combatFlags.p &= ~(1 << 7);
    actor->flags.p |= ACTOR_FLAG_COLLISION_CHECK | ACTOR_FLAG_TILE_COLLISION;
    actor->actionState = 1;
}
