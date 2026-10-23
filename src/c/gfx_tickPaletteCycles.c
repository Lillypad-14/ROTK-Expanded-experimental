#include "gfx.h"
#include "variables.h"

/**
 * Advance every active palette cycle by one frame.
 *
 * @romaddress 0x08016fac
 */
void gfx_unknown08016fac(void)
{
    u32 i;

    for (i = 0; i <= 11; i++)
    {
        PaletteCycleSlot *cycle = &g_PaletteCycles[i];

        if (cycle->flags & PALETTE_CYCLE_FLAG_ACTIVE)
        {
            gfx_stepPaletteCycle(cycle);
        }
    }
}
