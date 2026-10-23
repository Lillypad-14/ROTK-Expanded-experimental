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
#define TILE_IS_BLOCKING(type) ((u16)((type) - 1) <= 1)
#define SLIDE_NUDGE 0x5000

void actor_resolveTileCollision(Actor *actor)
{
    Vector2Fp16 renderCoords;
    Vector2Fp16 savedCoords;
    Vector2Int nudge;
    Vector2Int box;
    s32 left, right, top, bottom;
    s32 probeX, probeY, probeX2, probeY2;
    s32 edgeProbes = 0;
    u32 heading = 0;
    u32 facingLeft = 0;
    u32 savedFlags = actor->flags.p;
    u32 d32;

    g_TileCollisionFlags = 0;
    savedCoords = actor->coords;
    renderCoords = actor->renderCoords;

    if (actor->type == 0)
    {
        d32 = math_vector2_direction32(*(Vector2Fp16 *)&actor->velocity);
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
        facingLeft = actor->oam.hFlip;
    }
    (void)box;
    (void)nudge;
    (void)savedCoords;
    (void)renderCoords;
    (void)left;
    (void)right;
    (void)top;
    (void)bottom;
    (void)probeX;
    (void)probeY;
    (void)probeX2;
    (void)probeY2;
    (void)edgeProbes;
    (void)savedFlags;
}
