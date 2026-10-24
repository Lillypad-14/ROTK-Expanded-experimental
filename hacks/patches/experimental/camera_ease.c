#include "patches/experimental/experimental.h"

#include "gfx.h"
#include "vector2.h"

// World position of each player, one entry every 0x30 bytes.
#define PLAYER_POSITION(index) ((const Vector2Fp16 *)(0x03001a10 + (index) * 0x30))
// End of gfx_updatePlayerCameraScroll, the only caller that gets an eased position.
#define CAMERA_SCROLL_END 0x0803ecc0u
#define PLAYER_COUNT 2
#define ONE_PIXEL (1 << 16)
// Larger jumps are scene loads or teleports, which the camera follows at once.
#define SNAP_DISTANCE (48 << 16)

static Vector2Fp16 EasedPosition[PLAYER_COUNT];
// Bit per player, set once EasedPosition holds a position for that player.
static u8 EasedValid;

static bool32 isFar(fp16_16 gap)
{
    return gap > SNAP_DISTANCE || gap < -SNAP_DISTANCE;
}

static fp16_16 easeAxis(fp16_16 eased, fp16_16 target, u32 shift)
{
    fp16_16 gap = target - eased;

    if (gap < ONE_PIXEL && gap > -ONE_PIXEL)
    {
        return target;
    }
    return eased + (gap >> shift);
}

/** Copy a player's world position, eased toward it each tick when the camera asks. */
void cameraEase_getPlayerPosition(Vector2Fp16 *out, u8 playerIndex)
{
    u32 caller = (u32)__builtin_return_address(0);
    u32 strength = g_ExperimentalState.cameraEase;
    Vector2Fp16 *eased;
    u32 bit;

    *out = *PLAYER_POSITION(playerIndex);
    if (playerIndex >= PLAYER_COUNT || caller < (u32)gfx_updatePlayerCameraScroll ||
        caller >= CAMERA_SCROLL_END)
    {
        return;
    }
    bit = 1 << playerIndex;
    eased = &EasedPosition[playerIndex];
    if (strength == 0 || !(EasedValid & bit) || isFar(out->x - eased->x) || isFar(out->y - eased->y))
    {
        *eased = *out;
        EasedValid = strength == 0 ? EasedValid & ~bit : EasedValid | bit;
        return;
    }
    eased->x = easeAxis(eased->x, out->x, strength);
    eased->y = easeAxis(eased->y, out->y, strength);
    *out = *eased;
}
