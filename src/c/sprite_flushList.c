#include "player.h"
#include "sprite.h"
#include "system.h"
#include "variables.h"

/**
 * Flush an actor list: process each object, hide the unused OAM tail, wait for vblank.
 *
 * @romaddress 0x08000bbc
 */
void sprite_flushList(ActorList *list)
{
    Actor *actor = list->head;

    while (actor != NULL)
    {
        Actor *next;

        actor->flags.p = (actor->flags.p & ~ACTOR_FLAG_RENDER) | 0x82;
        if (actor->type == 13 && actor->variant == 1 && (u8)actor->as.switch_.currentFrame != 0)
        {
            PLAYER(actor->lastHitByPlayerIndex).combatFlags.p &= ~PLAYER_COMBAT_FLAG_WARRIORS_POOL;
        }
        next = actor->next;
        sprite_free(actor);
        actor = next;
    }
    sprite_endFrame();
    system_waitVBlank();
}
