#include "sprite.h"

/**
 * Make an actor's sprite use an animation descriptor and rewind it to frame 0. A different
 * descriptor marks the animation frame as changed.
 *
 * @romaddress 0x08001bf0
 */
void sprite_bindAnimation(Actor *actor, const SpriteAnimation *animation)
{
    if (actor->spriteAnimation != animation)
    {
        actor->spriteAnimation = animation;
        actor->flags.p |= ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
    }
    sprite_setAnimationFrame(actor, 0);
}
