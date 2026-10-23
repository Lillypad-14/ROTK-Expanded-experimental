#define sprite_attachSlot hdr_dummy
#include "sprite.h"
#undef sprite_attachSlot

/**
 * Attach an animation bank part to a sprite slot.
 *
 * @romaddress 0x08001bd4
 */
void sprite_attachSlot(Actor *actor, u8 slot, u8 partId, u8 paletteNumber)
{
    actor->sprite.slotPartIds[slot] = partId;
    actor->sprite.slotPaletteNumbers[slot] = paletteNumber;
}
