#include "gfx.h"
#include "variables.h"

void gfx_stepPaletteCycle(PaletteCycleSlot *cycle)
{
    s32 color;
    u32 flags;
    u32 f0;

    if (cycle->delayLeft == 0)
    {
        f0 = cycle->flags;
        if (f0 & PALETTE_CYCLE_FLAG_BACKWARD)
        {
            color = cycle->currentColor;
            if (color == cycle->firstColor)
                color = cycle->lastColor;
            else
                color--;
        }
        else
        {
            color = cycle->currentColor;
            if (color == cycle->lastColor)
                color = cycle->firstColor;
            else
                color++;
        }
        cycle->currentColor = color;
        flags = cycle->flags;
        if (flags & PALETTE_CYCLE_FLAG_PING_PONG)
        {
            u32 bit = PALETTE_CYCLE_FLAG_BACKWARD;
            s32 edge;

            if (flags & bit)
                edge = cycle->firstColor;
            else
                edge = cycle->lastColor;
            if (cycle->currentColor == edge)
                cycle->flags = flags ^ bit;
        }
        cycle->flags |= PALETTE_CYCLE_FLAG_COMMIT_PENDING;
        color = cycle->stepDelay;
    }
    else
    {
        color = cycle->delayLeft - 1;
    }
    cycle->delayLeft = color;
}
