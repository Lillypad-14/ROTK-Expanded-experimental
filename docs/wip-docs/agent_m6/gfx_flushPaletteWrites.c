#include "gfx.h"
#include "variables.h"

typedef struct PaletteWrite
{
    u16 firstColor;
    u16 colorCount;
    const u16 *colors;
} PaletteWrite;

// Stand-ins for the queue at 0x030034e0 and the palette buffer at 0x03001ac8.
#define g_PaletteWrites ((PaletteWrite *)g_PaletteFades)
#define PALETTE_BUFFER ((u16 *)0x03001ac8)

void gfx_loadPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount, u16 *buffer);

/**
 * Copy every queued palette write into the palette buffer and empty the queue.
 *
 * @romaddress 0x08016df8
 */
void gfx_flushPaletteWrites(void)
{
    u32 i = 0;

    if (g_PaletteWriteCount != 0)
    {
        PaletteWrite *write = g_PaletteWrites;

        do
        {
            gfx_loadPaletteColors(g_PaletteWrites[i].colors, write->firstColor, write->colorCount, PALETTE_BUFFER);
            write++;
            i++;
        } while (i < g_PaletteWriteCount);
    }
    g_PaletteWriteCount = 0;
}

void gfx_loadPaletteColors(const u16 *colors, u32 firstColor, u32 colorCount, u16 *buffer)
{
}
