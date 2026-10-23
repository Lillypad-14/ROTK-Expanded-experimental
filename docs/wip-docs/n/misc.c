#include "actor.h"
#include "gba_io.h"
#include "gfx.h"
#include "sprite.h"
#include "text.h"

void gfx_enableDisplayFlags(u16 mask)
{
    REG_DISPCNT |= mask;
}

void gfx_setBlendControl(u16 arg1, u16 arg2)
{
    REG_BLDCNT = arg1 | 0x40 | (arg2 << 8);
}

void text_objSetPos(void *obj, s32 xCenter, s32 y)
{
    s16 x = xCenter;

    sprite_setPosition(obj, (x - (((u16 *)obj)[16] >> 1)) << 16, y << 16);
}
