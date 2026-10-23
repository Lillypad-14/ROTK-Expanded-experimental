#include "actor.h"
#include "gba_io.h"
#include "gfx.h"
#include "math.h"
#include "player.h"
#include "sprite.h"
#include "variables.h"

#define VISIBILITY_MARGIN 40
#define VISIBILITY_MARGIN_WIDE 100

/**
 * Set the actor's on-screen flags from each player's camera viewport.
 *
 * @romaddress 0x08000c38
 */
void actor_updateVisibility(Actor *actor)
{
    Vector2Int camera;
    Vector2Int position;
    u32 playerIndex;
    u32 cameraCount;
    s32 marginX;
    s32 marginY;

    if (actor->field_0xfe == 1)
    {
        actor->flags.p |= ACTOR_FLAG_ON_SCREEN;
        actor->flags.p |= ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN;
    }
    if ((actor->flags.p & ACTOR_FLAG_ALWAYS_VISIBLE) || actor->listId == ACTOR_LIST_OVERLAY)
    {
        actor->flags.p |= ACTOR_FLAG_ON_SCREEN | ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN;
        return;
    }
    cameraCount = g_PlayerGlobals.activePlayerCount != 0 ? g_PlayerGlobals.activePlayerCount : 1;
    for (playerIndex = 0; playerIndex < cameraCount; playerIndex++)
    {
        gfx_getCameraPosition(&camera, playerIndex);
        math_vector2_fromFp16(&actor->coords, &position);
        if ((actor->flags.p & ACTOR_FLAG_RENDER) && (actor->field_0x100 & 0x10))
        {
            marginX = VISIBILITY_MARGIN_WIDE;
            marginY = VISIBILITY_MARGIN_WIDE;
        }
        else
        {
            marginX = VISIBILITY_MARGIN;
            marginY = VISIBILITY_MARGIN;
        }
        if (position.x >= camera.x - marginX && position.y >= camera.y - marginY &&
            position.x <= camera.x + DISPLAY_WIDTH + marginX &&
            position.y <= camera.y + DISPLAY_HEIGHT + marginY)
        {
            actor->flags.p |= ACTOR_FLAG_ON_SCREEN;
            if (playerIndex == g_PlayerGlobals.activePlayerIndex)
            {
                actor->flags.p |= ACTOR_FLAG_ON_ACTIVE_PLAYER_SCREEN;
            }
        }
    }
}
