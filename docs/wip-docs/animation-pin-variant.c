#include "sprite.h"
#include "match_hacks.h"

/**
 * Advance an actor's frame countdown and optional animation command stream.
 *
 * @romaddress 0x080022e4
 */
void actor_tickAnimation(Actor *actor)
{
    u32 remaining = actor->animationFramesLeft;
    u32 flags = actor->flags.p;

    if ((flags & ACTOR_FLAG_ANIMATION_PLAYING) && remaining != 0)
    {
        remaining--;
        if (remaining == 0)
        {
            MATCH_PIN(const SpriteAnimationCommand *, cursor, "r1") =
                (const SpriteAnimationCommand *)actor->animationScriptCursor;
            if (cursor == NULL)
            {
                s32 frame = ++actor->currentAnimationFrame;

                if (frame >= actor->spriteAnimation->frames->frameCount)
                {
                    actor->currentAnimationFrame = remaining;
                }
                if (frame == actor->spriteAnimation->frames->frameCount - 1 ||
                    actor->spriteAnimation->frames->frameCount == 1)
                {
                    actor->flags.p |= ACTOR_FLAG_ANIMATION_END;
                }
                remaining = actor->animationFrameDuration;
            }
            else if (!(flags & ACTOR_FLAG_ANIMATION_END))
            {
                const SpriteAnimationCommand *next = cursor + 1;
                u32 frame;

                actor->animationScriptCursor = (const u8 *)next;
                frame = cursor[1].frame;
                if (frame == SPRITE_ANIMATION_JUMP)
                {
                    MATCH_PIN(u32, jumpOffset, "r1") = next->argument;
                    jumpOffset *= sizeof(SpriteAnimationCommand);
                    next = (const SpriteAnimationCommand *)((u32)actor->animationJumpBase + jumpOffset);
                    actor->animationScriptCursor = (const u8 *)next;
                    frame = next->frame;
                }
                if (next->frame == SPRITE_ANIMATION_END)
                {
                    actor->flags.p |= ACTOR_FLAG_ANIMATION_END;
                }
                else if (next->frame == SPRITE_ANIMATION_SIGNAL)
                {
                    actor->field_0x78[7]++;
                    remaining = 1;
                }
                else
                {
                    actor->currentAnimationFrame = frame;
                    remaining = next->argument;
                }
            }
        }
        if (actor->previousAnimationFrame != actor->currentAnimationFrame)
        {
            sprite_setAnimationFrame(actor, actor->currentAnimationFrame);
            actor->flags.p |= ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
        }
        actor->animationFramesLeft = remaining;
    }
}
