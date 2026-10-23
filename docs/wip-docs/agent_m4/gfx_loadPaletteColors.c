#include "gba.h"
#include "gfx.h"
#include "memory.h"

typedef struct PaletteTarget
{
    u16 *shadow;
    u16 *ram;
} PaletteTarget;

void gfx_loadPaletteColors(const u16 *colors, u32 first, u32 count, PaletteTarget *target);
#define g_BgPaletteTarget ((PaletteTarget *)0x03001ac0)
#define g_ObjPaletteTarget ((PaletteTarget *)0x03001ac8)


/**
 * Copy raw colors into the BG palette shadow buffer and palette RAM.
 *
 * @romaddress 0x08016cbc
 */
void gfx_loadBgPaletteColors(const u16 *colors, u32 first, u32 count)
{
    gfx_loadPaletteColors(colors, first, count, g_BgPaletteTarget);
}

/**
 * Copy raw colors into the OBJ palette shadow buffer and palette RAM.
 *
 * @romaddress 0x08016ccc
 */
void gfx_loadObjPaletteColors(const u16 *colors, u32 first, u32 count)
{
    gfx_loadPaletteColors(colors, first, count, g_ObjPaletteTarget);
}

/**
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
