#include "lib/movement.h"
#include "terrain.h"
#include "variables.h"

#define COLLISION_CELL_PIXELS 8

bool hackLib_canLandAt(const Actor *actor, const Vector2Fp16 *position)
{
    Vector2Int size;
    s32 left, right, top, bottom, x, y, radius;

    if (actor == NULL || position == NULL || g_TerrainCollisionBlocks == NULL ||
        g_TerrainCollisionCells == NULL)
    {
        return FALSE;
    }
    terrain_getWorldSize(&size);
    if (actor->movementCollisionBox.xMin > actor->movementCollisionBox.xMax)
    {
        return FALSE;
    }
    radius = ABS(actor->movementCollisionBox.xMin);
    if (ABS(actor->movementCollisionBox.xMax) > radius)
    {
        radius = ABS(actor->movementCollisionBox.xMax);
    }
    // Reserve both horizontal orientations of the movement box.
    left = (position->x >> FP16_16_SHIFT) - radius;
    right = (position->x >> FP16_16_SHIFT) + radius;
    top = (position->y >> FP16_16_SHIFT) + actor->movementCollisionBox.yMin;
    bottom = (position->y >> FP16_16_SHIFT) + actor->movementCollisionBox.yMax;
    if (left > right || top > bottom || left < 0 || top < 0 || right >= size.x || bottom >= size.y)
    {
        return FALSE;
    }
    for (y = top / COLLISION_CELL_PIXELS; y <= bottom / COLLISION_CELL_PIXELS; y++)
    {
        for (x = left / COLLISION_CELL_PIXELS; x <= right / COLLISION_CELL_PIXELS; x++)
        {
            // Reject blocking and special terrain conservatively.
            if ((terrain_getCollisionAt(x * COLLISION_CELL_PIXELS, y * COLLISION_CELL_PIXELS) &
                 7) != 0)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

bool hackLib_findLanding(const Actor *actor, const Vector2Fp16 *preferred, Vector2Fp16 *result)
{
    static const s8 Offsets[][2] = {{0, 0},   {0, -8},  {0, 8},  {-8, 0}, {8, 0},
                                    {-8, -8}, {8, -8},  {-8, 8}, {8, 8},  {0, -16},
                                    {0, 16},  {-16, 0}, {16, 0}};
    u32 i;

    if (preferred == NULL || result == NULL)
    {
        return FALSE;
    }
    for (i = 0; i < ARRAY_COUNT(Offsets); i++)
    {
        Vector2Fp16 candidate = {preferred->x + Offsets[i][0] * FP16_16_ONE,
                                 preferred->y + Offsets[i][1] * FP16_16_ONE};

        if (hackLib_canLandAt(actor, &candidate))
        {
            *result = candidate;
            return TRUE;
        }
    }
    return FALSE;
}
