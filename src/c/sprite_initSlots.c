#include "sprite.h"

/**
 * Enable multi-sprite mode and init the actor's SpriteRender slots.
 *
 * @romaddress 0x08001b2c
 */
void sprite_initSlots(Actor *actor)
{
    u32 i;

    actor->spriteFlags |= 0x40;
    actor->sprite.animationBank = NULL;
    for (i = 0; i < 4; i++)
    {
        actor->sprite.slotPartIds[i] |= 0xff;
        actor->sprite.slotTileNumbers[i] |= 0xffff;
        actor->sprite.slotPaletteNumbers[i] = actor->oam.paletteNumber;
    }
}
