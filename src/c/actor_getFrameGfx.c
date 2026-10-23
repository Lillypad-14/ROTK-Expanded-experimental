#include "sprite.h"

/**
 * Return the current animation frame's tile stream, retaining its compression format.
 *
 * @romaddress 0x08001e24
 */
void *actor_getFrameGfx(Actor *actor)
{
    const SpriteAnimation *animation = actor->spriteAnimation;
    const SpriteFrameSetHeader *frames = animation->frames;
    u32 index = actor->currentAnimationFrame << 1;
    const u8 *base = (const u8 *)(frames + 1);
    u16 offset = *(const u16 *)(base + index);
    const SpriteFrame *frame =
        (const SpriteFrame *)((const u8 *)frames + (offset + sizeof(SpriteFrameSetHeader)));
    const u8 *tileOffset = (const u8 *)&frame->tileOffset;
    u32 low = tileOffset[0];
    u32 high = tileOffset[1];
    high <<= 8;
    high |= low;
    return (u8 *)animation->tiles + high;
}
