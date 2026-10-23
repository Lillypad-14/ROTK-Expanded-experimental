#include "math.h"

/**
 * Copy a vector.
 *
 * @romaddress 0x08032920
 */
void math_vector2_copy(const Vector2Fp16 *src, Vector2Fp16 *dst)
{
    dst->x = src->x;
    dst->y = src->y;
}
