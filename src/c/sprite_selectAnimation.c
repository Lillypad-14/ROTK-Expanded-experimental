#include "game.h"
#include "gfx.h"
#include "sprite.h"
#include "variables.h"

/**
 * Select an actor's animation from a bank and upload its palette.
 *
 * @param pose column of the bank
 * @param facingVariant SpriteAnimation within the column
 *
 * @romaddress 0x08000d58
 */
void sprite_selectAnimation(Actor *actor, void *animationBank, s8 pose, s8 facingVariant)
{
    const SpriteAnimation *animation =
        (const SpriteAnimation *)(*(u8 **)((u8 *)animationBank + ((pose << 24) >> 22)) + ((facingVariant << 24) >> 20));

    actor->sprite.facingVariant = facingVariant;
    actor->sprite.animationBank = animationBank;
    actor->sprite.pose = pose;
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
