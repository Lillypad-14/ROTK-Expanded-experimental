#define sprite_ensureOamMatrix hdr_dummy
#include "sprite.h"
#undef sprite_ensureOamMatrix

/**
 * Get the actor's OBJ affine matrix index, allocating one when it has none.
 *
 * @romaddress 0x08001a6c
 */
u16 sprite_ensureOamMatrix(Actor *actor)
{
    u8 mode = actor->oam.affineMode;
    u16 index;

    if (mode == OAM_AFFINE_ON || mode == OAM_AFFINE_DOUBLE)
    {
        index = sprite_getOamMatrixIndex(&actor->oam);
    }
    else
    {
        index = sprite_allocOamMatrix();
        sprite_setOamMatrixIndex(&actor->oam, index);
    }
    return index;
}
