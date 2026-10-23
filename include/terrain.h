#pragma once

#include "vector2.h"

void terrain_getWorldSize(Vector2Int *size);

/** Collision byte at pixel coordinates. Out-of-bounds coordinates return zero. */
u8 terrain_getCollisionAt(s32 x, s32 y);
