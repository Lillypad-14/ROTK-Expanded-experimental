#include "gfx.h"
#include "variables.h"

/**
 * Stop every palette cycle and fade and drop the queued palette writes.
 *
 * @romaddress 0x08016d70
 */
void gfx_clearPaletteAnims(void)
{
    gfx_unknown08016f74();
    gfx_unknown080170e8();
    g_PaletteWriteCount = 0;
}

/**
 * Per-frame step of every active palette cycle and palette fade.
 *
 * @romaddress 0x08016d88
 */
void gfx_tickPaletteAnims(void)
{
    gfx_unknown08016fac();
    gfx_unknown08017130();
}
