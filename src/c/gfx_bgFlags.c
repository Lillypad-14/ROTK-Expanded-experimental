#include "gfx.h"
#include "variables.h"

/**
 * Clear bit 0 of the global BG flags.
 *
 * @romaddress 0x08012f3c
 */
void gfx_clearBgFlags0(void)
{
    g_Bg.flags &= ~1;
}

/**
 * Set bit 0 of the global BG flags.
 *
 * @romaddress 0x08012f50
 */
void gfx_setBgFlags0(void)
{
    g_Bg.flags |= 1;
}

/**
 * Read the masked global BG flags.
 *
 * @romaddress 0x08012f60
 */
u32 gfx_getBgFlags(u32 mask)
{
    return g_Bg.flags & mask;
}

/**
 * Clear bits of the global BG flags.
 *
 * @romaddress 0x08012f70
 */
void gfx_clearBgFlags(u32 mask)
{
    g_Bg.flags &= ~mask;
}

/**
 * Set bits of the global BG flags.
 *
 * @romaddress 0x08012f80
 */
void gfx_setBgFlags(u32 mask)
{
    g_Bg.flags |= mask;
}
