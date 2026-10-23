#include "actor.h"

/**
 * Set the actor's velocity from its facing and move speed.
 *
 * @romaddress 0x08001c40
 */
void actor_applyFacingVelocity(Actor *actor)
{
    Vector2Fp16 velocity;

    actor_setVelocityFromDirection8(&velocity, actor->flags.p, actor->facing, actor->moveSpeed);
    actor->velocity.x = velocity.x;
    actor->velocity.y = velocity.y;
}
