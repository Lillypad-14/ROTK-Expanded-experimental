#include "gba.h"
#include "gfx.h"
#include "variables.h"

/**
 * Horizontal-only gfx_setBgScrollSpeed.
 *
 * @romaddress 0x08012d10
 */
void gfx_setBgScrollSpeedX(u32 layer, fp16_16 speedX)
{
    g_Bg.layers[layer].speed.x = speedX;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_SCROLL;
}

/**
 * Vertical-only gfx_setBgScrollSpeed.
 *
 * @romaddress 0x08012d34
 */
void gfx_setBgScrollSpeedY(u32 layer, fp16_16 speedY)
{
    g_Bg.layers[layer].speed.y = speedY;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_SCROLL;
}

/**
 * Copy a BG layer's RAM tilemap to VRAM when it is marked dirty.
 *
 * @romaddress 0x08012d58
 */
void gfx_flushBgTilemap(u32 layer)
{
    u32 bit = 2 << layer;
    u32 flags = g_Bg.flags;

    if (flags & bit)
    {
        BgLayer *bg;

        g_Bg.flags = flags & ~bit;
        bg = &g_Bg.layers[layer];
        bios_cpuSet(bg->tilemapBuffer, bg->tilemap, 0x400);
    }
}

/**
 * Clear a BG layer's color effect values.
 *
 * @romaddress 0x08012d90
 */
void gfx_clearBgColorFx(u32 layer)
{
    BgLayer *bg = &g_Bg.layers[layer];

    gfx_clearBgColorFxEven(layer);
    gfx_clearBgColorFxOdd(layer);
    bg->flags &= ~BG_LAYER_FLAG_COLOR_FX;
}

/**
 * Clear the even color effect values of a BG layer.
 *
 * @romaddress 0x08012dc0
 */
void gfx_clearBgColorFxEven(u32 layer)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[0] = 0;
    bg->colorFx[2] = 0;
    bg->colorFx[4] = 0;
}

/**
 * Clear the odd color effect values of a BG layer.
 *
 * @romaddress 0x08012de0
 */
void gfx_clearBgColorFxOdd(u32 layer)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[1] = 0;
    bg->colorFx[3] = 0;
    bg->colorFx[5] = 0;
}

/**
 * Set all color effect values of a BG layer.
 *
 * @romaddress 0x08012e00
 */
void gfx_setBgColorFx(u32 layer, const BgColorFx *values)
{
    BgLayer *bg = &g_Bg.layers[layer];

    *(BgColorFx *)bg->colorFx = *values;
    bg->flags |= BG_LAYER_FLAG_COLOR_FX;
}

/**
 * Set the even color effect values of a BG layer.
 *
 * @romaddress 0x08012e28
 */
void gfx_setBgColorFxEven(u32 layer, u8 a, u16 b, u16 c)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[0] = a << 8;
    bg->colorFx[2] = b;
    bg->colorFx[4] = c;
}

/**
 * Set the odd color effect values of a BG layer.
 *
 * @romaddress 0x08012e50
 */
void gfx_setBgColorFxOdd(u32 layer, u8 a, u16 b, u16 c)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[1] = a << 8;
    bg->colorFx[3] = b;
    bg->colorFx[5] = c;
}

/**
 * Set the first color effect pair of a BG layer.
 *
 * @romaddress 0x08012e78
 */
void gfx_setBgColorFxPair0(u32 layer, u16 a, u16 b)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[0] = a << 8;
    bg->colorFx[1] = b << 8;
}

/**
 * Set the second color effect pair of a BG layer.
 *
 * @romaddress 0x08012e98
 */
void gfx_setBgColorFxPair1(u32 layer, u16 a, u16 b)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[2] = a;
    bg->colorFx[3] = b;
}

/**
 * Set the third color effect pair of a BG layer.
 *
 * @romaddress 0x08012eb4
 */
void gfx_setBgColorFxPair2(u32 layer, u16 a, u16 b)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->colorFx[4] = a;
    bg->colorFx[5] = b;
}
