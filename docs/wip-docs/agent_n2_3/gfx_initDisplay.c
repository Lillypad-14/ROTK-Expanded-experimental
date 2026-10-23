#define gfx_initDisplay hdr_dummy
#include "game.h"
#include "gba_io.h"
#include "gfx.h"
#include "player.h"
#include "variables.h"
#undef gfx_initDisplay

// Needs name at 0x0803f250 (void, u8 playerIndex): e.g. gfx_resetCamera
void gfx_resetCamera(u8 playerIndex);

#define BG_LAYERS_BASE ((u8 *)0x03003260)

/**
 * Reset the display to the given DISPCNT mode with OBJ 1D mapping and reset BG2/BG3 scale.
 *
 * @romaddress 0x08012478
 */
void gfx_initDisplay(u16 mode)
{
    u8 *base;
    u16 *a;
    u16 *b;

    REG_DISPCNT = mode | 0x40;
    base = BG_LAYERS_BASE;
    a = (u16 *)(base + 0x126);
    *a = 0x100;
    b = (u16 *)(base + 0x196);
    *b = 0x100;
    if (!(g_GameFlags.p & GAME_FLAG_REQUEST_OVERLAY))
    {
        gfx_resetCamera(ACTIVE_PLAYER_INDEX);
    }
}

void gfx_resetCamera(u8 playerIndex)
{
}
