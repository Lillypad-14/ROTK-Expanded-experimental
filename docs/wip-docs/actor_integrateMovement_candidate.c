#include "sprite.h"
#include "math.h"

#define MOVEMENT_DRAG 0xc000

/**
 * Integrate acceleration and velocity into an actor's render position.
 *
 * @romaddress 0x080023c0
 */
void actor_integrateMovement(Actor *actor)
{
    u32 flags = actor->flags.p;
    Vector2Fp16 acceleration = ((ActorMovementScratch *)actor->field_0xbc)->acceleration;
    Vector2Fp16 velocity = actor->velocity;
    Vector2Fp16 position = actor->coords;

    math_vector2_add(&velocity, &acceleration, &velocity);
    if (!(flags & (ACTOR_FLAG_28 | ACTOR_FLAG_18 | ACTOR_FLAG_IGNORE_SOFT_TILES | ACTOR_FLAG_10)) &&
        actor->type != ACTOR_TYPE_NPC && actor->field_0xf6[1] == 5)
    {
        if (velocity.x < -MOVEMENT_DRAG)
        {
            velocity.x += MOVEMENT_DRAG;
        }
        else if (velocity.x > MOVEMENT_DRAG)
        {
            velocity.x -= MOVEMENT_DRAG;
        }
        if (velocity.y < -MOVEMENT_DRAG)
        {
            velocity.y += MOVEMENT_DRAG;
        }
        else if (velocity.y > MOVEMENT_DRAG)
        {
            velocity.y -= MOVEMENT_DRAG;
        }
    }
    math_vector2_add(&position, &velocity, &position);
    actor->renderCoords = position;
    actor->velocity = velocity;
}
