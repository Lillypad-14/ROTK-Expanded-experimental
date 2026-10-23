#include "gba.h"
#include "gfx.h"
#include "memory.h"

typedef struct PaletteTarget
{
    u16 *shadow;
    u16 *ram;
} PaletteTarget;

/**
 * Copy raw colors into a palette shadow buffer and its palette RAM.
 *
 * @romaddress 0x08016f28
 */
void gfx_loadPaletteColors(const u16 *colors, u32 first, u32 count, PaletteTarget *target)
{
    u16 *dst;

    count *= 2;
    first *= 2;
    dst = (u16 *)((u8 *)target->shadow + first);
    memory_memcpy(dst, colors, count);
    dst = (u16 *)((u8 *)target->ram + first);
    bios_cpuSet(colors, dst, (count << 10) >> 11);
}
