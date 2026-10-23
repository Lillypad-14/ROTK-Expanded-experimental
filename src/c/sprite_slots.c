#include "sprite.h"

/**
 * Release a sprite slot by marking it free.
 *
 * @romaddress 0x08001b90
 */
void sprite_detachSlot(Actor *actor, u8 slot)
{
    actor->sprite.slotPartIds[slot] = 0xff;
}

/**
 * Set bit 0 of a sprite slot's flags.
 *
 * @romaddress 0x08001ba4
 */
void sprite_setSlotFlag0(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] |= 1 << 0;
}

/**
 * Clear bit 0 of a sprite slot's flags.
 *
 * @romaddress 0x08001bbc
 */
void sprite_clearSlotFlag0(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] &= ~(1 << 0);
}

/**
 * Show animation bank part partId in a sprite slot with the given OAM palette.
 *
 * @romaddress 0x08001bd4
 */
void sprite_attachSlot(Actor *actor, u8 slot, u8 partId, u8 paletteNumber)
{
    actor->sprite.slotPartIds[slot] = partId;
    actor->sprite.slotPaletteNumbers[slot] = paletteNumber;
}
