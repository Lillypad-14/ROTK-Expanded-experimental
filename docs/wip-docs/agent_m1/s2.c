#include "gfx.h"
#include "variables.h"

void gfx_setBgScrollSpeed(u32 layer, Vector2Fp16 speed)
{
    u8 *base = (u8 *)g_BgLayers - 4;
    u32 off = layer * 0x70;
    u8 *p;
    p = base + 0x2c;
    *(s32 *)(p + off) = speed.x;
    p = base + 0x30;
    *(s32 *)(p + off) = speed.y;
    p = base + 0x18;
    *(u32 *)(p + off) |= 1;
}
