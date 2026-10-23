#include "gfx.h"
#include "variables.h"

void gfx_stepPaletteCycle(PaletteCycleSlot *cycle)
{
    s32 color;
    u32 flags;

    if (cycle->delayLeft == 0)
    {
        if (cycle->flags & PALETTE_CYCLE_FLAG_BACKWARD)
        {
            color = cycle->currentColor;
            if (color == cycle->firstColor)
            {
                color = cycle->lastColor;
            }
            else
            {
                color--;
            }
        }
        else
        {
            color = cycle->currentColor;
            if (color == cycle->lastColor)
            {
                color = cycle->firstColor;
            }
            else
            {
                color++;
            }
        }
        cycle->currentColor = color;
        flags = cycle->flags;
        if (flags & PALETTE_CYCLE_FLAG_PING_PONG)
        {
            s32 edge;

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
            {
                u32 back = PALETTE_CYCLE_FLAG_BACKWARD;
                cycle->flags = flags ^ back;
            }
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
