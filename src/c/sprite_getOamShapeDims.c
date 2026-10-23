#include "sprite.h"
#include "variables.h"

/**
 * Look up the pixel width and height of an OBJ shape/size pair.
 *
 * @romaddress 0x0800c018
 */
void sprite_getOamShapeDims(u32 shape, u32 size, Vector2Int *out)
{
    out->x = OamShapeDims[shape][size][0];
    out->y = OamShapeDims[shape][size][1];
}
