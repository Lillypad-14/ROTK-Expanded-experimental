#include "gfx.h"
#include "memory.h"
#include "variables.h"

void gfx_unknown08016f74(void)
{
    u32 i;
    u8 *cycle;

    g_PaletteCycleCount = 0;
    for (i = 0, cycle = (u8 *)g_PaletteCycles; i <= 11; cycle += 8, i++)
    {
        gfx_clearPaletteCycle(cycle);
    }
}

void gfx_unknown080170e8(void)
{
    u32 i;
    u8 *fade;

    g_PaletteFadeCount = 0;
    for (i = 0, fade = (u8 *)g_PaletteFades; i <= 3; fade += 16, i++)
    {
        gfx_clearPaletteFade(fade);
    }
}
