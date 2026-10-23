#include "gfx.h"
#include "memory.h"
#include "variables.h"

void gfx_clearPaletteCycle(void *cycle)
{
    memory_memset(cycle, 0, 8);
}
