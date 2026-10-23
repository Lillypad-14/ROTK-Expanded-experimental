#define actor_getOffsetPosition hdr_dummy
#include "actor.h"
#include "sprite.h"
#include "variables.h"
#undef actor_getOffsetPosition

/**
 * Hotspot of the current animation frame as a world position, for FX anchoring.
 *
 * @param offX sprite slot to read (-1 for the main sprite)
 *
 * @return 1 when a hotspot was found
 *
 * @romaddress 0x0800103c
 */
u32 actor_getOffsetPosition(Actor *actor, u32 offY, Vector2Fp16 *out, s32 offX)
{
    const SpriteAnimation *anim;
    const SpriteFrameSetHeader *header;
    const SpriteHotspot *hotspot;
    s32 facingVariant;
    s32 part;
    s32 pose;

    if (offX == -1)
    {
        anim = actor->spriteAnimation;
    }
    else
    {
        facingVariant = *(s8 *)&actor->sprite.facingVariant;
        part = *(s8 *)&actor->sprite.slotPartIds[offX];
        if (part == -1)
        {
            return 0;
        }
        pose = *(s8 *)&actor->sprite.pose;
        anim = ((const SpriteAnimation *(*)[7])actor->sprite.animationBank)[part][pose] + facingVariant;
    }
    header = anim->frames;
    if (offY >= header->hotspotCount)
    {
        return 0;
    }
    hotspot = (const SpriteHotspot *)((u8 *)header + 0xc +
                                      *(u16 *)((u8 *)header + 0xc + actor->currentAnimationFrame * 2) + 8 + offY * 2);
    if (hotspot->x == -127)
    {
        return 0;
    }
    out->x = hotspot->x;
    out->y = actor->coords.y + (hotspot->y << 16);
    if (actor->facing > 4)
    {
        out->x = actor->coords.x - (hotspot->x << 16);
    }
    else
    {
        out->x = actor->coords.x + (hotspot->x << 16);
    }
    return 1;
}
