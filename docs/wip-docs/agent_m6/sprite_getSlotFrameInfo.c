#include "sprite.h"

/**
 * Slot equivalent of actor_getAnimationFrame: read the slot's current frame.
 *
 * @param out receives the frame's cell width, then height
 * @return the frame's tile data size in bytes
 *
 * @romaddress 0x08001100
 */
u16 sprite_getSlotFrameInfo(Actor *actor, u32 *out, u8 slot)
{
    s32 pose = *(s8 *)&actor->sprite.pose;
    s32 partId = *(s8 *)&actor->sprite.slotPartIds[slot];
    s32 facingVariant = *(s8 *)&actor->sprite.facingVariant;
    const SpriteFrameSetHeader *frames;
    u8 flags;
    u8 backwards;
    s32 frameIndex;
    const u8 *frame;

    frames = ((const SpriteAnimation *)*(u32 *)((u8 *)actor->sprite.animationBank +
                                                  (partId * 7 + pose) * 4) +
              facingVariant)
                 ->frames;
    flags = actor->sprite.slotFlags[slot];
    backwards = flags & 4;
    if (backwards || (flags & 8))
    {
        frameIndex = actor->currentAnimationFrame - 1;
        if (frameIndex < 0)
        {
            frameIndex = backwards ? frames->frameCount - 1 : 0;
        }
    }
    else
    {
        frameIndex = actor->currentAnimationFrame;
    }
    frame = (const u8 *)frames + *(const u16 *)((const u8 *)(frames + 1) + frameIndex * 2) + sizeof(*frames);
    out[0] = frame[2];
    out[1] = frame[3];
    return frame[6] | (frame[7] << 8);
}
