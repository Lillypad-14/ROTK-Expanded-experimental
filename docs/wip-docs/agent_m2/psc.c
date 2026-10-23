#include "gfx.h"
#include "variables.h"

void gfx_stepPaletteCycle(PaletteCycleSlot *cycle)
{
    u32 color;
    u32 flags;
    u32 delay = cycle->delayLeft;

    if (delay == 0)
    {
        if (cycle->flags & PALETTE_CYCLE_FLAG_BACKWARD)
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
            u32 edge;

            if (flags & PALETTE_CYCLE_FLAG_BACKWARD)
            {
                color = cycle->currentColor;
                edge = cycle->firstColor;
            }
            else
            {
                color = cycle->currentColor;
                edge = cycle->lastColor;
            }
            if (color == edge)
                cycle->flags = flags ^ PALETTE_CYCLE_FLAG_BACKWARD;
        }
        cycle->flags |= PALETTE_CYCLE_FLAG_COMMIT_PENDING;
        delay = cycle->stepDelay;
    }
    else
    {
        delay--;
    }
    cycle->delayLeft = delay;
}
