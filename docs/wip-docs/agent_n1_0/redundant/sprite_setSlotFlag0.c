#define sprite_setSlotFlag0 hdr_dummy
#include "sprite.h"
#undef sprite_setSlotFlag0

/**
 * @romaddress 0x08001ba4
 */
void sprite_setSlotFlag0(Actor *actor, u8 slot)
{
    actor->sprite.slotFlags[slot] |= 1;
}
