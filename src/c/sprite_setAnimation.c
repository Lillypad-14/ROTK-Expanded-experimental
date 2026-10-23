#include "game.h"
#include "gfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Point an actor's sprite at an animation descriptor and restart it on frame 0. A descriptor with a
 * palette uploads it to the actor's OBJ palette slot.
 *
 * @romaddress 0x08001ac4
 */
void sprite_setAnimation(Actor *actor, const SpriteAnimation *animation)
{
    actor->animationFrameDuration = animation->frameDuration;
    sprite_bindAnimation(actor, animation);
    if (animation->palette != NULL)
    {
        if (g_GameFlags.p & GAME_FLAG_SCENE_CHANGE)
        {
            gfx_loadObjPaletteColors(animation->palette, actor->oam.paletteNumber << 4, 16);
        }
        else
        {
            gfx_queuePaletteWrite(animation->palette, actor->oam.paletteNumber << 4, 16);
        }
    }
    actor->flags.p &= ~ACTOR_FLAG_ANIMATION_END;
}
