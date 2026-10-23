#include "variables.h"
#include "libc.h"
#include "sprite.h"

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
 * Set a sprite's OAM scale immediately.
 *
 * @romaddress 0x080019b4
 */
void sprite_setScale(Actor *actor, s16 scaleX, s16 scaleY, Angle16 angle)
{
    u16 index = sprite_ensureOamMatrix(actor);
    OamMatrixParams *base = g_OamMatrixParams;

    base[index].scaleX = scaleX;
    base[index].scaleY = scaleY;
    base[index].angle = angle;
    if (scaleX == 0)
    {
        scaleX = 1;
    }
    actor->sprite.invScaleX = 0x40000 / scaleX;
    actor->sprite.invScaleY = 0x40000 / scaleY;
    actor->oam.affineMode = OAM_AFFINE_DOUBLE;
}

u16 sprite_ensureOamMatrix(Actor *actor)
{
    return 0;
}
