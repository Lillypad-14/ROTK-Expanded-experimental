#include "sprite.h"

/**
 * Disable sprite scaling and release its OBJ affine matrix.
 *
 * @romaddress 0x08001a2c
 */
void sprite_disableScale(Actor *actor)
{
    u8 mode = actor->oam.affineMode;

    if (mode == OAM_AFFINE_ON || mode == OAM_AFFINE_DOUBLE)
    {
        u16 index;

        actor->oam.affineMode = OAM_AFFINE_OFF;
        index = sprite_getOamMatrixIndex(&actor->oam);
        sprite_setOamMatrixIndex(&actor->oam, 0);
        sprite_freeOamMatrix(index);
    }
}
