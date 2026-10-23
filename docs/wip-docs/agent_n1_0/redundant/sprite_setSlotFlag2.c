#define sprite_setSlotFlag2 hdr_dummy
#include "sprite.h"
#undef sprite_setSlotFlag2

/**
 * @romaddress 0x08001d10
 */
void sprite_setSlotFlag2(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] |= 4;
}
