#include "sprite.h"

/**
 * Teleport an actor: set both its world coordinates and its render coordinates to (x, y).
 *
 * @romaddress 0x08001c20
 */
void sprite_setPosition(Actor *actor, u32 x, u32 y)
{
    actor->coords.x = x;
    actor->coords.y = y;
    actor->renderCoords.x = x;
    actor->renderCoords.y = y;
}
