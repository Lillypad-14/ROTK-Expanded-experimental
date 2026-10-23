#include "gfx.h"
#include "variables.h"

/**
 * Copy a player's camera position (top-left world pixel of the viewport) to out.
 *
 * @romaddress 0x0803f214
 */
void gfx_getCameraPosition(Vector2Int *out, u8 playerIndex)
{
    *out = g_CameraPositions[playerIndex];
}

/**
 * Set a player's camera position (top-left world pixel of the viewport).
 *
 * @romaddress 0x0803f230
 */
void gfx_setCameraPosition(const Vector2Int *position, u8 playerIndex)
{
    g_CameraPositions[playerIndex] = *position;
}
