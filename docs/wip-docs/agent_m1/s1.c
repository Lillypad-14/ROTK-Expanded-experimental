#include "gfx.h"
#include "variables.h"

void gfx_setBgScrollSpeed(u32 layer, Vector2Fp16 speed)
{
    u8 *base = (u8 *)g_BgLayers - 4;
    u32 off = layer * 0x70;
    *(s32 *)(base + 0x2c + off) = speed.x;
    *(s32 *)(base + 0x30 + off) = speed.y;
    *(u32 *)(base + 0x18 + off) |= 1;
}
