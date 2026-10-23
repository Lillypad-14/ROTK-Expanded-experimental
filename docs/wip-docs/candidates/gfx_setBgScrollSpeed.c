#include "gfx.h"
#include "variables.h"
void gfx_setBgScrollSpeed(u32 layer, Vector2Fp16 speed)
{
    g_BgLayers[layer].scrollSpeed.x = speed.x;
    g_BgLayers[layer].scrollSpeed.y = speed.y;
    g_BgLayers[layer].flags |= 1;
}
