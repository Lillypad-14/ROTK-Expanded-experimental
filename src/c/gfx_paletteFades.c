#include "gfx.h"
#include "memory.h"
#include "variables.h"

/**
 * Clear every palette fade slot.
 *
 * @romaddress 0x080170e8
 */
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

/**
 * Clear one palette fade slot and drop the active fade count.
 *
 * @romaddress 0x08017110
 */
void gfx_clearPaletteFade(void *fade)
{
    memory_memset(fade, 0, 16);
    if (g_PaletteFadeCount != 0)
    {
        g_PaletteFadeCount--;
    }
}
