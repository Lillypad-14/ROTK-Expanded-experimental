#include "sprite.h"

/**
 * Read one animation frame's cell pixel size.
 *
 * @param sizeOut receives width, then height
 *
 * @romaddress 0x0800bfb8
 */
void sprite_getAnimationFrameSize(const SpriteAnimation *animation, u32 *sizeOut, u8 frameIndex)
{
    const SpriteFrameSetHeader *frames = animation->frames;
    u32 index = frameIndex << 1;
    const u8 *base = (const u8 *)(frames + 1);
    u16 offset = *(const u16 *)(base + index);
    const SpriteFrame *frame = (const SpriteFrame *)((const u8 *)(frames + 1) + offset);

    sizeOut[0] = frame->width;
    sizeOut[1] = frame->height;
}
