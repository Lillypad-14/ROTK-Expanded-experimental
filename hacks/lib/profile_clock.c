#include "gba_io.h"
#include "lib/profile.h"
#include "system.h"
#include "variables.h"

#define LINES_PER_FRAME 228
#define VBLANK_FIRST_LINE 160

u32 hackLib_profileClock(void)
{
    u32 before;
    u32 after;
    u32 line;
    volatile u32 *counter = &g_EngineState->vblankCounter;

    do
    {
        before = *counter;
        line = REG_VCOUNT;
        after = *counter;
    } while (before != after);
    return before * LINES_PER_FRAME +
           (line + LINES_PER_FRAME - VBLANK_FIRST_LINE) % LINES_PER_FRAME;
}
