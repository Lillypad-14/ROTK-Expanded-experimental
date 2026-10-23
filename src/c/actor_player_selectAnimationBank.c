#include "actor.h"
#include "player.h"
#include "variables.h"

/**
 * Point the actor at the hero's bank in HeroMoveAnimationBanks (0), HeroMeleeAnimationBanks (1),
 * or HeroCastAnimationBanks (2). Other bank sets leave the actor unchanged.
 *
 * @romaddress 0x080273e4
 */
void actor_player_selectAnimationBank(Actor *actor, u8 heroId, u8 bankSet)
{
    switch (bankSet)
    {
    case 0:
        actor->as.combat.head.player.animationBank = (void *)HeroMoveAnimationBanks[heroId];
        break;
    case 1:
        actor->as.combat.head.player.animationBank = (void *)HeroMeleeAnimationBanks[heroId];
        break;
    case 2:
        actor->as.combat.head.player.animationBank = (void *)HeroCastAnimationBanks[heroId];
        break;
    }
}
