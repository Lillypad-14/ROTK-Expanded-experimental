#include "gba.h"
#include "gfx.h"
#include "memory.h"

/**
 * Copy raw colors into a palette shadow buffer and its palette RAM.
 *
 * @romaddress 0x08016f28
 */
void gfx_loadPaletteColors(const u16 *colors, u32 first, u32 count, PaletteBank *bank)
{
    u16 *dst;

    count *= 2;
    first *= 2;
    dst = (u16 *)((u8 *)bank->shadow + first);
    memory_memcpy(dst, colors, count);
    dst = (u16 *)((u8 *)bank->ram + first);
    bios_cpuSet(colors, dst, (count << 10) >> 11);
}
