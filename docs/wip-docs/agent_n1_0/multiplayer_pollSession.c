#define multiplayer_pollSession hdr_dummy
#include "gba_io.h"
#include "multiplayer.h"
#include "variables.h"
#undef multiplayer_pollSession

/**
 * Poll link progress and timeout.
 *
 * @return 0 when no session is pending, 6 when it timed out, else the link state
 *
 * @romaddress 0x08046bc4
 */
s32 multiplayer_pollSession(void)
{
    if ((s8)g_MpLinkState[5] == -1)
    {
        return 0;
    }
    if ((s8)g_MpLinkState[5] >= g_UnknownMpValue_03006dc4)
    {
        g_MpLinkState[4] = 0;
        g_MpLinkState[5] |= 0xff;
        g_MpMessageMask |= 1 << 16;
        REG_IE &= ~0x80;
        REG_SIOMLT_SEND = 0xffff;
        return 6;
    }
    return g_UnknownMpValue_03006db0;
}
