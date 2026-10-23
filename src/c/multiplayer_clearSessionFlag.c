#include "multiplayer.h"
#include "variables.h"

/**
 * Clear the low bit of the session flags when clear is nonzero, otherwise set it.
 *
 * @romaddress 0x08046d8c
 */
void multiplayer_clearSessionFlag(u8 clear)
{
    if (clear)
    {
        g_MpSessionFlags &= ~1;
    }
    else
    {
        g_MpSessionFlags |= 1;
    }
}
