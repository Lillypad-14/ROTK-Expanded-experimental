#include "gba_io.h"
#include "gfx.h"
#include "math.h"
#include "variables.h"

/**
 * Stop and reset one BG layer's transform state and scroll registers.
 *
 * @romaddress 0x08012b1c
 */
void gfx_resetBgTransform(u32 layer)
{
    BgLayer *bg = &g_Bg.layers[layer];

    bg->flags = 0;
    math_vector2_copy(&g_Vector2Zero, &bg->field_0x30);
    math_vector2_copy(&g_Vector2Zero, &bg->speed);
    math_vector2_copy(&g_Vector2Zero, &bg->position);
    ((volatile u16 *)REG_ADDR_BG0HOFS)[layer * 2] = 0;
    ((volatile u16 *)REG_ADDR_BG0VOFS)[layer * 2] = 0;
}

/**
 * Set a BG layer's scroll position.
 *
 * @romaddress 0x08012b74
 */
void gfx_setBgPosition(u32 layer, fp16_16 x, fp16_16 y)
{
    g_Bg.layers[layer].position.x = x;
    g_Bg.layers[layer].position.y = y;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_COMMIT;
}

/**
 * Set a BG layer's horizontal scroll position.
 *
 * @romaddress 0x08012ba4
 */
void gfx_setBgPositionX(u32 layer, fp16_16 x)
{
    g_Bg.layers[layer].position.x = x;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_COMMIT;
}

/**
 * Set a BG layer's vertical scroll position.
 *
 * @romaddress 0x08012bc8
 */
void gfx_setBgPositionY(u32 layer, fp16_16 y)
{
    g_Bg.layers[layer].position.y = y;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_COMMIT;
}

/**
 * Set a BG layer's rotation angle and per-tick rotation speed.
 *
 * @romaddress 0x08012bec
 */
void gfx_setBgRotationAnim(u32 layer, u16 rotation, u16 speed)
{
    g_Bg.layers[layer].rotation = rotation;
    g_Bg.layers[layer].rotationSpeed = speed;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_ROTATE;
}

/**
 * Set a BG layer's rotation angle.
 *
 * @romaddress 0x08012c18
 */
void gfx_setBgRotation(u32 layer, u16 rotation)
{
    g_Bg.layers[layer].rotation = rotation;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_MATRIX_DIRTY;
}

/**
 * Set the amount a BG layer's rotation advances each tick.
 *
 * @romaddress 0x08012c3c
 */
void gfx_setBgRotationSpeed(u32 layer, u16 speed)
{
    g_Bg.layers[layer].rotationSpeed = speed;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_ROTATE;
}

/**
 * Set the screen point a BG layer rotates and scales around.
 *
 * @romaddress 0x08012c5c
 */
void gfx_setBgPivot(u32 layer, s16 x, s16 y)
{
    g_Bg.layers[layer].pivotX = x;
    g_Bg.layers[layer].pivotY = y;
}

/**
 * Set the limits a BG layer's animated zoom bounces between.
 *
 * @romaddress 0x08012c7c
 */
void gfx_setBgZoomRange(u32 layer, s16 min, s16 max)
{
    g_Bg.layers[layer].zoomMin = min;
    g_Bg.layers[layer].zoomMax = max;
}

/**
 * Set how much a BG layer's zoom changes each tick.
 *
 * @romaddress 0x08012c9c
 */
void gfx_setBgZoomStep(u32 layer, s16 step)
{
    g_Bg.layers[layer].zoomStep = step;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_ZOOM_ANIM;
}

/**
 * Set a BG layer's zoom.
 *
 * @romaddress 0x08012cbc
 */
void gfx_setBgZoom(u32 layer, s16 zoom)
{
    g_Bg.layers[layer].zoom = zoom;
    g_Bg.layers[layer].flags |= BG_LAYER_FLAG_MATRIX_DIRTY;
}
