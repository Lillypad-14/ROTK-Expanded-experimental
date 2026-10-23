#include "gfx.h"
#include "memory.h"
#include "variables.h"

void gfx_clearPaletteFade(void *fade)
{
    memory_memset(fade, 0, 16);
    if (g_PaletteFadeCount != 0)
    {
        g_PaletteFadeCount--;
    }
}
