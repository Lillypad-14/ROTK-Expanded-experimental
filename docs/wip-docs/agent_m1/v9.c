#include "actor.h"
#include "math.h"
extern s32 Direction8MoveVectors[16];
extern s32 Direction8MoveVectorsSteep[16];

void actor_setVelocityFromDirection8(Vector2Fp16 *velocityOut, u32 flags, s32 direction, s32 speed)
{
    if (flags & (1 << 24)) {
        u8 *table = (u8 *)Direction8MoveVectorsSteep;
        s32 off = direction * 8;
        velocityOut->x = math_multiplyFp16(*(s32 *)(table + off), speed);
        table += 4;
        velocityOut->y = math_multiplyFp16(*(s32 *)(table + off), speed);
    } else {
        u8 *table = (u8 *)Direction8MoveVectors;
        s32 off = direction * 8;
        velocityOut->x = math_multiplyFp16(*(s32 *)(table + off), speed);
        table += 4;
        velocityOut->y = math_multiplyFp16(*(s32 *)(table + off), speed);
    }
}
