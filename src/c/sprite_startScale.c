#include "libc.h"
#include "sprite.h"
#include "variables.h"

/**
 * Start a linear x/y scale-over-time run.
 *
 * @romaddress 0x0800151c
 */
void sprite_startScale(Actor *actor, s32 startX, s32 startY, s32 targetX, s32 targetY, s32 frames)
{
    s32 scaleX;
    s32 scaleY;
    s32 x;
    s32 y;
    u16 index;
    OamMatrixParams *base;

    actor->sprite.scaleX = startX;
    actor->sprite.scaleY = startY;
    actor->sprite.scaleDeltaX = (targetX - startX) / frames;
    actor->sprite.scaleDeltaY = (targetY - startY) / frames;
    actor->sprite.scaleFramesLeft = frames;
    x = actor->sprite.scaleX << 8;
    y = actor->sprite.scaleY << 8;
    scaleY = y >> 16;
    scaleX = x >> 16;
    index = sprite_ensureOamMatrix(actor);
    base = g_OamMatrixParams;
    base[index].scaleX = scaleX;
    base[index].scaleY = scaleY;
    base[index].angle = 0;
    if (scaleX == 0)
    {
        scaleX = 1;
    }
    actor->sprite.invScaleX = 0x40000 / (s16)scaleX;
    actor->sprite.invScaleY = 0x40000 / (s16)scaleY;
    actor->oam.affineMode = OAM_AFFINE_DOUBLE;
}
