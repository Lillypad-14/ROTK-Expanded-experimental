#include "gfx.h"
#include "variables.h"

/**
 * Set a BG layer's scroll speed and mark it dirty.
 *
 * @romaddress 0x08012ce0
 */
void gfx_setBgScrollSpeed(u32 layer, Vector2Fp16 speed)
{
    g_Bg.layers[layer].speed.x = speed.x;
    g_Bg.layers[layer].speed.y = speed.y;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_SCROLL;
}
