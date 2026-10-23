#include "actor.h"
#include "player.h"
#include "stats.h"
#include "variables.h"

/**
 * Add delta to a player's current HP (clamped to STAT_MAX_HP) and mark the HUD HP display dirty.
 * On damage with flinchOnDamage set, a player in action state 1, 2, or 5 is knocked into the hurt
 * action state.
 *
 * @romaddress 0x08026148
 */
void player_addHp(u8 playerIndex, s16 delta, bool flinchOnDamage)
{
    PLAYER(playerIndex).currentHp += delta;
    if (PLAYER(playerIndex).currentHp > PLAYER_STAT(playerIndex, STAT_MAX_HP))
    {
        PLAYER(playerIndex).currentHp = PLAYER_STAT(playerIndex, STAT_MAX_HP);
    }
    g_PlayerHuds[playerIndex].flags.p |= PLAYERHUD_FLAG_HP_DIRTY;
    if (flinchOnDamage && delta < 0)
    {
        Actor *actor = PLAYER(playerIndex).ownerActor;

        if (actor->actionState == 1 || actor->actionState == 5 || actor->actionState == 2)
        {
            actor->actionState = ACTOR_STATE_HURT;
            actor->field_0x64[0] |= 2;
        }
    }
}
