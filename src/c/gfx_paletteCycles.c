#include "gfx.h"
#include "memory.h"
#include "variables.h"

/**
 * Clear every palette cycle slot.
 *
 * @romaddress 0x08016f74
 */
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

/**
 * Clear one palette cycle slot.
 *
 * @romaddress 0x08016f9c
 */
void gfx_clearPaletteCycle(void *cycle)
{
    memory_memset(cycle, 0, 8);
}
