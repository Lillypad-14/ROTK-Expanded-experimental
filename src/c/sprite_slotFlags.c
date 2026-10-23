#include "sprite.h"

/**
 * Set a sprite slot's draw order (0..7).
 *
 * @romaddress 0x08001cd0
 */
void sprite_setSlotDrawOrder(Actor *actor, u8 slot, u8 drawOrder)
{
    actor->sprite.slotDrawOrders[slot] = drawOrder;
}

/**
 * Set bit 1 of a sprite slot's flags.
 *
 * @romaddress 0x08001ce0
 */
void sprite_setSlotFlag1(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] |= 1 << 1;
}

/**
 * Clear bit 1 of a sprite slot's flags.
 *
 * @romaddress 0x08001cf8
 */
void sprite_clearSlotFlag1(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] &= ~(1 << 1);
}

/**
 * Set bit 2 of a sprite slot's flags.
 *
 * @romaddress 0x08001d10
 */
void sprite_setSlotFlag2(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] |= 1 << 2;
}

/**
 * Clear bit 2 of a sprite slot's flags.
 *
 * @romaddress 0x08001d28
 */
void sprite_clearSlotFlag2(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] &= ~(1 << 2);
}

/**
 * Copy an actor's OAM x and y position to out.
 *
 * @romaddress 0x08001d40
 */
void sprite_getOamPosition(Actor *actor, Vector2Int *out)
{
    out->x = actor->oam.x;
    out->y = actor->oam.y;
}
