#include "gba.h"
#include "gfx.h"

typedef struct PaletteBank
{
    const u16 *shadow; // palette buffer the cycles rotate
    u16 *ram;          // palette RAM the buffer is copied to
} PaletteBank;

// BG bank at 0x03001ac0, OBJ bank right after it.
#define g_PaletteBanks ((PaletteBank *)0x03001ac0)

/**
 * Copy a palette cycle's rotated colors from the palette buffer to palette RAM, if a step is pending.
 *
 * @romaddress 0x0801704c
 */
void gfx_commitPaletteCycle(PaletteCycleSlot *cycle)
{
    if (cycle->flags & PALETTE_CYCLE_FLAG_COMMIT_PENDING)
    {
        PaletteBank *bank;

        cycle->flags &= ~PALETTE_CYCLE_FLAG_COMMIT_PENDING;
        bank = (cycle->flags & PALETTE_CYCLE_FLAG_OBJ_PALETTE) ? &g_PaletteBanks[1] : &g_PaletteBanks[0];
        if (cycle->currentColor == cycle->firstColor)
        {
            bios_cpuSet(bank->shadow + cycle->firstColor, bank->ram + cycle->firstColor, cycle->field_0x06);
        }
        else
        {
            u32 count = cycle->lastColor - cycle->currentColor + 1;

            bios_cpuSet(bank->shadow + cycle->firstColor, bank->ram + cycle->currentColor, count & 0x1fffff);
            count = cycle->currentColor - cycle->firstColor;
            bios_cpuSet((u16 *)((u8 *)bank->shadow + (cycle->lastColor - count + 1) * 2), bank->ram + cycle->firstColor,
                        count & 0x1fffff);
        }
    }
}
