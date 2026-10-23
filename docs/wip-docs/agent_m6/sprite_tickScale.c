#include "libc.h"
#include "sprite.h"
#include "variables.h"

typedef struct OamMatrixParams
{
    s16 scaleX;
    s16 scaleY;
    Angle16 angle;
    u16 padding;
} OamMatrixParams;

#define g_OamMatrixParams ((OamMatrixParams *)g_OamMatrixAllocFlags)

u16 sprite_ensureOamMatrix(Actor *actor);

/**
 * Advance the scale run one frame.
 *
 * @romaddress 0x080015ec
 */
void sprite_tickScale(Actor *actor)
{
    s32 scaleX;
    s32 scaleY;
    s32 x;
    s32 y;
    u16 index;
    OamMatrixParams *base;

    actor->sprite.scaleFramesLeft--;
    actor->sprite.scaleX += actor->sprite.scaleDeltaX;
    actor->sprite.scaleY += actor->sprite.scaleDeltaY;
    scaleY = actor->sprite.scaleY;
    x = actor->sprite.scaleX << 8;
    y = scaleY << 8;
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

u16 sprite_ensureOamMatrix(Actor *actor)
{
    return 0;
}
