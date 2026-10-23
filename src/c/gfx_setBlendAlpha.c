#include "gba_io.h"
#include "gfx.h"
#include "variables.h"

/**
 * Set the alpha blend coefficients and write them to the blend alpha register.
 *
 * @param eva first-target coefficient (0-16)
 * @param evb second-target coefficient (0-16)
 *
 * @romaddress 0x0803df00
 */
void gfx_setBlendAlpha(u16 eva, u16 evb)
{
    g_BlendEva.value = eva << 8;
    g_BlendEvb.value = evb << 8;
    REG_BLDALPHA = (evb << 8) | eva;
}
