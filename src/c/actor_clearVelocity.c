#include "actor.h"

/**
 * Zero the actor's displacement velocity.
 *
 * @romaddress 0x08001c70
 */
void actor_clearVelocity(Actor *actor)
{
    actor->velocity.x = 0;
    actor->velocity.y = 0;
}
