#include "sprite.h"
#include "match_hacks.h"

/**
 * Follow a target's render position and priority, with an optional one-pixel Y offset.
 *
 * @romaddress 0x08003540
 */
void actor_updateRenderFollow(Actor *actor)
{
    MATCH_PIN(u32, flags, "r1");

    actor->renderCoords = actor->renderFollowTarget->renderCoords;
    actor->oam.bgPriority = actor->renderFollowTarget->oam.bgPriority;
    flags = actor->spriteFlags;
    if (flags & ACTOR_SPRITE_FLAG_FOLLOW_DOWN)
    {
        actor->renderCoords.y += FP16_16_ONE;
    }
    else if (flags & ACTOR_SPRITE_FLAG_FOLLOW_UP)
    {
        actor->renderCoords.y -= FP16_16_ONE;
    }
}
