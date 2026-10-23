#include "variables.h"
#include "libc.h"
#include "sprite.h"

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
