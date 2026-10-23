#include "gba.h"
#include "gfx.h"

// Raw addresses keep the ROM literal pool layout.
#define BG_PALETTE_BANK ((PaletteBank *)0x03001ac0)
#define OBJ_PALETTE_BANK ((PaletteBank *)0x03001ac8)

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
        bank = (cycle->flags & PALETTE_CYCLE_FLAG_OBJ_PALETTE) ? OBJ_PALETTE_BANK : BG_PALETTE_BANK;
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
