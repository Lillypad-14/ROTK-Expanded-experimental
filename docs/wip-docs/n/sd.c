#include "sprite.h"
#include "variables.h"

void sprite_getOamShapeDims(u32 shape, u32 size, Vector2Int *out)
{
    out->x = OamShapeDims[shape][size][0];
    out->y = OamShapeDims[shape][size][1];
}
