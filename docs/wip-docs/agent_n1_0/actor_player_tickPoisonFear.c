#define actor_player_tickPoisonFear hdr_dummy
#include "actor.h"
#include "actor/player.h"
#include "player.h"
#include "variables.h"
#undef actor_player_tickPoisonFear

/**
 * Tick the poison (-2 HP / 32 frames) and fear (-3 spirit / 16 frames) status timers.
 *
 * @romaddress 0x08026370
 */
void actor_player_tickPoisonFear(Actor *actor)
{
    u8 playerIndex = actor->as.combat.playerIndex;
    u32 *flags = &g_PlayerHuds[playerIndex].flags.p;

    if (*flags & PLAYERHUD_FLAG_POISON)
    {
        if (--actor->as.combat.venomDuration <= 0)
        {
            *flags = (*flags & ~PLAYERHUD_FLAG_POISON) | PLAYERHUD_FLAG_12;
        }
        else if ((actor->as.combat.venomDuration & 0x1f) == 0)
        {
            player_addHp(playerIndex, -2, FALSE);
        }
    }
    flags = &g_PlayerHuds[playerIndex].flags.p;
    if (*flags & PLAYERHUD_FLAG_FEAR)
    {
        if (--actor->as.combat.fearDuration <= 0)
        {
            *flags = (*flags & ~PLAYERHUD_FLAG_FEAR) | PLAYERHUD_FLAG_12;
        }
        else if ((actor->as.combat.fearDuration & 0xf) == 0)
        {
            player_addSpirit(playerIndex, -3);
        }
    }
}
