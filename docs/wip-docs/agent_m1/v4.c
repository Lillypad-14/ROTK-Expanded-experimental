#include "actor.h"
#include "math.h"
extern s32 Direction8MoveVectors[16];
extern s32 Direction8MoveVectorsSteep[16];

void actor_setVelocityFromDirection8(Vector2Fp16 *velocityOut, u32 flags, s32 direction, s32 speed)
{
    s32 *table = (flags & (1 << 24)) ? Direction8MoveVectorsSteep : Direction8MoveVectors;
    u8 *t = (u8 *)table;
    s32 off = direction * 8;
    velocityOut->x = math_multiplyFp16(*(s32 *)(t + off), speed);
    t += 4;
    velocityOut->y = math_multiplyFp16(*(s32 *)(t + off), speed);
}
