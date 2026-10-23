#include "sprite.h"

/**
 * Read a frame's tile compression mode, preferring RL when both format bits are set.
 *
 * @romaddress 0x0800bfdc
 */
u8 sprite_getFrameCompression(const SpriteFrameSetHeader *frames, u16 frameIndex)
{
    u32 index = frameIndex << 1;
    const u8 *base = (const u8 *)(frames + 1);
    u16 offset = *(const u16 *)(base + index);
    const SpriteFrame *frame = (const SpriteFrame *)((const u8 *)(frames + 1) + offset);
    if (frame->flags.p & SPRITE_FRAME_RL)
    {
        return SPRITE_COMPRESSION_RL;
    }
    if (frame->flags.p & SPRITE_FRAME_LZ77)
    {
        return SPRITE_COMPRESSION_LZ77;
    }
    return SPRITE_COMPRESSION_RAW;
}
