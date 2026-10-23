#define gfx_queuePaletteWrite hdr_dummy
#include "gfx.h"
#include "variables.h"
#undef gfx_queuePaletteWrite

// Needs config: g_PaletteWrites at 0x030034e0, PaletteWrite[], 8 bytes each.
typedef struct PaletteWrite
{
    u16 firstColor;
    u16 colorCount;
    const u16 *colors;
} PaletteWrite;

#define g_PaletteWrites ((PaletteWrite *)0x030034e0)

/**
 * Queue colorCount raw colors for palette RAM at firstColor.
 *
 * @romaddress 0x08016cdc
 */
void gfx_queuePaletteWrite(const u16 *colors, u32 firstColor, u32 colorCount)
{
    u32 index = g_PaletteWriteCount;

    g_PaletteWrites[index].colors = colors;
    g_PaletteWrites[index].firstColor = firstColor;
    g_PaletteWrites[index].colorCount = colorCount;
    g_PaletteWriteCount++;
}
