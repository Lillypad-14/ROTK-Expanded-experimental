#include "actor.h"
#include "math.h"
extern s32 Direction8MoveVectors[16];
extern s32 Direction8MoveVectorsSteep[16];

void actor_setVelocityFromDirection8(Vector2Fp16 *velocityOut, u32 flags, s32 direction, s32 speed)
{
    s32 *table;
    s32 off;
    if (flags & (1 << 24))
        table = Direction8MoveVectorsSteep;
    else
        table = Direction8MoveVectors;
    off = direction * 2;
    velocityOut->x = math_multiplyFp16(table[off], speed);
    velocityOut->y = math_multiplyFp16(table[off + 1], speed);
}
