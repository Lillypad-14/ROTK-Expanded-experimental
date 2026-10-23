#include "actor.h"
#include "math.h"
extern s32 Direction8MoveVectors[16];
extern s32 Direction8MoveVectorsSteep[16];

void actor_setVelocityFromDirection8(Vector2Fp16 *velocityOut, u32 flags, s32 direction, s32 speed)
{
    u8 *table;
    s32 off;
    if ((flags & (1 << 24)) != 0)
        table = (u8 *)Direction8MoveVectorsSteep;
    else
        table = (u8 *)Direction8MoveVectors;
    off = direction * 8;
    velocityOut->x = math_multiplyFp16(*(s32 *)(table + off), speed);
    table += 4;
    velocityOut->y = math_multiplyFp16(*(s32 *)(table + off), speed);
}
