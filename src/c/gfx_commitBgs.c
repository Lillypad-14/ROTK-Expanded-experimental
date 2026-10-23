#include "gfx.h"

/**
 * Reset the transform of all four backgrounds.
 *
 * @romaddress 0x08012b04
 */
void gfx_commitBgs(void)
{
    u32 i;

    for (i = 0; i <= 3; i++)
    {
        gfx_resetBgTransform(i);
    }
}
