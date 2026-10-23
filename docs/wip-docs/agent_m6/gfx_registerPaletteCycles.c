#define gfx_registerPaletteCycles gfx_registerPaletteCycles_hdr
#include "gfx.h"
#include "scene.h"
#include "variables.h"
#undef gfx_registerPaletteCycles

u32 gfx_allocPaletteCycle(void);

// Stand-in for the mask of cycles already registered, 0x03004164.
#define g_PaletteCycleRegistered g_UnknownMissionState_03004164

/**
 * Register every cycle of a PaletteCycleList into the palette-cycle slots.
 *
 * @romaddress 0x08016bdc
 */
u32 gfx_registerPaletteCycles(const PaletteCycleList *table)
{
    u32 slot = 0;
    s32 i = 0;

    if ((s32)slot < (s32)*(u16 *)table)
    {
        PaletteCycleSlot *next = g_PaletteCycles;
        const PaletteCycle *cycle = table->cycles;

        do
        {
            PaletteCycleSlot *entry;
            u8 first = cycle->firstIndex;
            u8 count = cycle->count;
            u8 framesPerStep = cycle->framesPerStep;
            u8 flags = cycle->flags.p;

            slot = gfx_allocPaletteCycle();
            entry = &g_PaletteCycles[slot];
            entry->flags = flags;
            entry->stepDelay = framesPerStep;
            entry->delayLeft = 0;
            entry->firstColor = first;
            entry->currentColor = first;
            entry->lastColor = first + count - 1;
            entry->field_0x06 = count;
            entry->field_0x07 = count;
            if (g_SceneCurrent.args[0] == 0)
            {
                g_PaletteCycleRegistered |= 1 << i;
            }
            else if (!(g_PaletteCycleRegistered & (1 << i)))
            {
                next->flags &= ~1;
            }
            next++;
            i++;
            cycle++;
        } while (i < (s32)*(u16 *)table);
    }
    return slot;
}

u32 gfx_allocPaletteCycle(void)
{
    return 0;
}
