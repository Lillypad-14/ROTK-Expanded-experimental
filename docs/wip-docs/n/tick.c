#include "gfx.h"
#include "variables.h"

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
