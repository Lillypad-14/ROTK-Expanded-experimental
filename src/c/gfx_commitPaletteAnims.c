#include "game.h"
#include "gfx.h"
#include "variables.h"

/**
 * Commit every active palette cycle and palette fade to palette RAM.
 *
 * @romaddress 0x08016d98
 */
void gfx_commitPaletteAnims(void)
{
    u32 i;

    for (i = 0; i < 12; i++)
    {
        PaletteCycleSlot *cycle = &g_PaletteCycles[i];

        if (cycle->flags & 1)
        {
            gfx_commitPaletteCycle(cycle);
        }
    }
    for (i = 0; i < 4; i++)
    {
        u8 *fade = g_PaletteFades[i];

        if ((fade[0] & 1) && !(g_GameFlags.p & GAME_FLAG_OVERLAY))
        {
            gfx_commitPaletteFade(fade);
        }
    }
}
