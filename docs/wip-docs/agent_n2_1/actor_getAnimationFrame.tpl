#include "actor.h"
#include "sprite.h"
#include "variables.h"

/**
 * Size of the current animation frame and its gfx byte size.
 *
 * @return gfx byte size of the frame
 *
 * @romaddress 0x08001d54
 */
u16 actor_getAnimationFrame(Actor *actor, u32 *out)
{
    const SpriteFrameSet *set = (const SpriteFrameSet *)actor->spriteAnimation->frames;
    const u8 *frame;
    @@
    out[0] = frame[2];
    out[1] = frame[3];
    return frame[6] | (frame[7] << 8);
}
