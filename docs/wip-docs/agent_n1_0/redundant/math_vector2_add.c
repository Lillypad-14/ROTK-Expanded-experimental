#define math_vector2_add hdr_dummy
#include "math.h"
#undef math_vector2_add

/**
 * Componentwise a + b.
 *
 * @romaddress 0x080328f0
 */
void math_vector2_add(const Vector2Fp16 *a, const Vector2Fp16 *b, Vector2Fp16 *out)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
}
