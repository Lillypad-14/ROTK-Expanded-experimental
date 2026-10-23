#include "actor.h"
#include "math.h"
#include "sprite.h"
#include "terrain.h"
#include "variables.h"

enum TileCollisionFlag
{
    TILE_COLLISION_PROBE_A = 1 << 0,
    TILE_COLLISION_PROBE_B = 1 << 4,
    TILE_COLLISION_ANY = 1 << 8,
    TILE_COLLISION_EDGE = 1 << 12,
};

#define TILE_TYPE_AT(x, y) (TileCollisionTypes[terrain_getCollisionAt((x), (y)) & 7])
#define TILE_IS_BLOCKING(type) ((u16)((type)-1) <= 1)
#define SLIDE_NUDGE 0x5000

void actor_resolveTileCollision(Actor *actor)
{
    Vector2Fp16 renderCoords;
    Vector2Fp16 savedCoords;
    Vector2Fp16 nudge;
    s32 left, right, top, bottom;
    s32 probeX, probeY, probeX2, probeY2;
    s32 edgeProbes;
    u32 heading;
    u32 facingLeft;
    u32 savedFlags;
    u32 d32;
    s32 y;
    s32 tile;

    g_TileCollisionFlags = 0;
    edgeProbes = 0;
    heading = 0;
    facingLeft = 0;
    savedFlags = actor->flags.p;
    savedCoords = actor->coords;
    renderCoords = actor->renderCoords;

    if (actor->type == 0)
    {
        d32 = math_vector2_direction32(actor->velocity);
        if (d32 == 0)
        {
            facingLeft = heading;
        }
        else if (d32 == 8)
        {
            heading = 2;
        }
        else if (d32 == 16)
        {
            heading = 4;
        }
        else if (d32 == 24)
        {
            heading = 6;
            facingLeft = 1;
        }
        else if (d32 - 1 <= 6)
        {
            heading = 1;
        }
        else if (d32 - 9 <= 6)
        {
            heading = 3;
        }
        else if (d32 - 17 <= 6)
        {
            heading = 5;
            facingLeft = 1;
        }
        else if (d32 - 25 <= 6)
        {
            heading = 7;
            facingLeft = 1;
        }
    }
    else
    {
        heading = math_vector2_direction8(*(Vector2Int *)&actor->velocity);
        facingLeft = (actor->oam.matrixIndex & OAM_MATRIX_H_FLIP) != 0;
    }

    y = (s16)(renderCoords.y >> 16);
    top = y + actor->movementCollisionBox.yMin;
    bottom = y + actor->movementCollisionBox.yMax;
    if (facingLeft == 1)
    {
        left = (renderCoords.x >> 16) - actor->movementCollisionBox.xMax;
        right = (renderCoords.x >> 16) - actor->movementCollisionBox.xMin;
    }
    else
    {
        right = (renderCoords.x >> 16) + actor->movementCollisionBox.xMax;
        left = (renderCoords.x >> 16) + actor->movementCollisionBox.xMin;
    }

    switch (heading)
    {
    case 0:
        probeX = left;
        probeY = top;
        probeX2 = right;
        probeY2 = top;
        edgeProbes = (right - left) >> 3;
        break;
    case 1:
        probeX = left + ((right - left) >> 1);
        probeY = top;
        probeX2 = right;
        probeY2 = top + ((bottom - top) >> 1);
        break;
    case 2:
        probeX = right;
        probeY = top;
        probeX2 = right;
        probeY2 = bottom;
        edgeProbes = (bottom - top) >> 3;
        break;
    case 3:
        probeX = left + ((right - left) >> 1);
        probeY = bottom;
        probeX2 = right;
        probeY2 = top + ((bottom - top) >> 1);
        break;
    case 4:
        probeX = left;
        probeY = bottom;
        probeX2 = right;
        probeY2 = bottom;
        edgeProbes = (right - left) >> 3;
        break;
    case 5:
        probeX = left + ((right - left) >> 1);
        probeY = bottom;
        probeX2 = left;
        probeY2 = top + ((bottom - top) >> 1);
        break;
    case 6:
        probeX = left;
        probeY = top;
        probeX2 = left;
        probeY2 = bottom;
        edgeProbes = (bottom - top) >> 3;
        break;
    case 7:
        probeX = left + ((right - left) >> 1);
        probeY = top;
        probeX2 = left;
        probeY2 = top + ((bottom - top) >> 1);
        break;
    }

    g_TileCollisionProbeTypes[0] = TILE_TYPE_AT(probeX, probeY);
    g_TileCollisionProbeTypes[1] = TILE_TYPE_AT(probeX2, probeY2);

    if (actor->type == 0 && edgeProbes != 0)
    {
        u32 ignoreSoft = savedFlags & ACTOR_FLAG_IGNORE_SOFT_TILES;

        do
        {
            if (heading == 0 || heading == 4)
            {
                probeX += 8;
            }
            else
            {
                probeY += 8;
            }
            tile = TILE_TYPE_AT(probeX, probeY);
            g_TileCollisionCenterType = tile;
            if (TILE_IS_BLOCKING(tile))
            {
                if (ignoreSoft)
                {
                    if (tile == 2)
                    {
                        g_TileCollisionFlags |= TILE_COLLISION_EDGE;
                    }
                }
                else
                {
                    g_TileCollisionFlags |= TILE_COLLISION_EDGE;
                }
                break;
            }
        } while (--edgeProbes != 0);
    }

    if (g_TileCollisionProbeTypes[0] == 2 ||
        (g_TileCollisionProbeTypes[0] == 1 && !(savedFlags & ACTOR_FLAG_IGNORE_SOFT_TILES)))
    {
        g_TileCollisionFlags |= TILE_COLLISION_PROBE_A | TILE_COLLISION_ANY;
    }
    if (g_TileCollisionProbeTypes[1] == 2 ||
        (g_TileCollisionProbeTypes[1] == 1 && !(savedFlags & ACTOR_FLAG_IGNORE_SOFT_TILES)))
    {
        g_TileCollisionFlags |= TILE_COLLISION_PROBE_B | TILE_COLLISION_ANY;
    }

    if (heading & 1)
    {
        if (actor->type == 0)
        {
            probeX -= 8;
            tile = TILE_TYPE_AT(probeX, probeY);
            g_TileCollisionCenterType = tile;
            if (TILE_IS_BLOCKING(tile))
            {
                g_TileCollisionFlags |= TILE_COLLISION_PROBE_A;
            }
            probeX += 16;
            tile = TILE_TYPE_AT(probeX, probeY);
            g_TileCollisionCenterType = tile;
            if (TILE_IS_BLOCKING(tile))
            {
                g_TileCollisionFlags |= TILE_COLLISION_PROBE_A;
            }
        }
        if (g_TileCollisionFlags & TILE_COLLISION_PROBE_A)
        {
            actor->renderCoords.y = savedCoords.y;
            actor->velocity.y = 0;
        }
        if (g_TileCollisionFlags & TILE_COLLISION_PROBE_B)
        {
            actor->renderCoords.x = savedCoords.x;
            actor->velocity.x = 0;
        }
    }
    else
    {
        nudge.x = 0;
        nudge.y = 0;
        if (g_TileCollisionFlags & TILE_COLLISION_PROBE_A)
        {
            if (heading == 0 || heading == 4)
            {
                nudge.x = SLIDE_NUDGE;
            }
            else
            {
                nudge.y = SLIDE_NUDGE;
            }
            math_vector2_add(&savedCoords, &nudge, &actor->renderCoords);
        }
        if (g_TileCollisionFlags & TILE_COLLISION_PROBE_B)
        {
            if (heading == 0 || heading == 4)
            {
                nudge.x = -SLIDE_NUDGE;
            }
            else
            {
                nudge.y = -SLIDE_NUDGE;
            }
            math_vector2_add(&savedCoords, &nudge, &actor->renderCoords);
        }
        if ((g_TileCollisionFlags & (TILE_COLLISION_PROBE_A | TILE_COLLISION_PROBE_B)) ==
            (TILE_COLLISION_PROBE_A | TILE_COLLISION_PROBE_B))
        {
            actor->renderCoords.x = savedCoords.x;
            actor->renderCoords.y = savedCoords.y;
            actor->velocity.x = 0;
            actor->velocity.y = 0;
        }
        if ((g_TileCollisionFlags & (TILE_COLLISION_EDGE | TILE_COLLISION_ANY)) ==
            TILE_COLLISION_EDGE)
        {
            actor->renderCoords.x = savedCoords.x;
            actor->renderCoords.y = savedCoords.y;
            actor->velocity.x = 0;
            actor->velocity.y = 0;
            g_TileCollisionFlags |= TILE_COLLISION_ANY;
        }
    }

    {
        Vector2Int position;

        math_vector2_fromFp16(&actor->renderCoords, &position);
        tile = terrain_getCollisionAt(position.x, position.y);
        g_TileCollisionCenterType = tile;
    }
    if (actor->actionState != ACTOR_STATE_UNKNOWN_11 || actor->oam.bgPriority != 3)
    {
        actor->oam.bgPriority = (tile >> 3) + 2;
    }
    actor_unknown08033e64(actor);
}
