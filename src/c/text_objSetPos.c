#include "actor.h"
#include "sprite.h"
#include "text.h"

/**
 * Move a text object so it is horizontally centered on xCenter.
 *
 * @romaddress 0x08047d7c
 */
void text_objSetPos(void *obj, s32 xCenter, s32 y)
{
    s16 x = xCenter;

    sprite_setPosition(obj, (x - (((u16 *)obj)[16] >> 1)) << 16, y << 16);
}
