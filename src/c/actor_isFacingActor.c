#include "actor.h"
#include "math.h"

/**
 * Whether self's facing points at target.
 *
 * @romaddress 0x08001250
 */
bool actor_isFacingActor(Actor *self, Actor *target)
{
    Vector2Int selfPos;
    Vector2Int targetPos;
    Vector2Int delta;
    Direction8 dir;
    Direction8 facing;

    math_vector2_fromFp16(&self->coords, &selfPos);
    math_vector2_fromFp16(&target->coords, &targetPos);
    delta.x = targetPos.x - selfPos.x;
    delta.y = targetPos.y - selfPos.y;
    dir = math_vector2_direction8(delta);
    facing = self->facing;
    switch (dir)
    {
    case 0:
        if (facing == 0)
            return TRUE;
        break;
    case 1:
        if (facing == 1)
            return TRUE;
        break;
    case 2:
        if (facing == 2)
            return TRUE;
        break;
    case 3:
        if (facing == 3)
            return TRUE;
        break;
    case 4:
        if (facing == 4)
            return TRUE;
        break;
    case 5:
        if (facing == 5)
            return TRUE;
        break;
    case 6:
        if (facing == 6)
            return TRUE;
        break;
    case 7:
        if (facing == 7)
            return TRUE;
        break;
    default:
        break;
    }
    return FALSE;
}
