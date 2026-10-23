#include "gfx.h"
#include "variables.h"

typedef struct PaletteWrite
{
    u16 firstColor;
    u16 colorCount;
    const u16 *colors;
} PaletteWrite;

// Stand-in for the queue at 0x030034e0.
#define g_PaletteWrites ((PaletteWrite *)g_PaletteFades)

/**
 * Queue colorCount raw colors for palette RAM at firstColor.
 *
 * @romaddress 0x08016cdc
 */
void gfx_queuePaletteWrite(const u16 *colors, u32 firstColor, u32 colorCount)
{
    g_PaletteWrites[g_PaletteWriteCount].colors = colors;
    g_PaletteWrites[g_PaletteWriteCount].firstColor = firstColor;
    g_PaletteWrites[g_PaletteWriteCount].colorCount = colorCount;
    g_PaletteWriteCount++;
}
