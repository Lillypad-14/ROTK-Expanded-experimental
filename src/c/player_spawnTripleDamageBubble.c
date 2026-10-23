#include "actor.h"
#include "fx.h"
#include "player.h"
#include "sfx.h"
#include "variables.h"

/**
 * Spawn a 300-frame triple-damage buff bubble on the player (STAT_TRIPLE_DAMAGE_PROC_PERCENT
 * proc); sets PLAYER_COMBAT_FLAG_TRIPLE_DAMAGE and Player.shieldBubble.
 *
 * @romaddress 0x08045ee4
 */
Actor *player_spawnTripleDamageBubble(Actor *playerActor)
{
    u8 playerIndex = playerActor->as.combat.playerIndex;
    Actor *fx = fx_spawnTimedOnActor(playerActor, 300, 4, 7, 0);

    if (fx == NULL)
        return NULL;
    {
        PLAYER(playerIndex).shieldBubble = fx;
        fx->spriteFlags |= 0x20;
        fx->type = 0x68;
        fx->tickCallback = player_tripleDamageBubble_tick;
        fx->triggerCallbacks[0] = NULL;
        fx->parentActor = playerActor;
        PLAYER(playerIndex).combatFlags.p |= PLAYER_COMBAT_FLAG_TRIPLE_DAMAGE;
        sfx_play(15);
    }
    return fx;
}
