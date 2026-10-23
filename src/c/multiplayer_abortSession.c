#include "gba_io.h"
#include "multiplayer.h"
#include "variables.h"

/**
 * Stop the link: disable serial and timer interrupts and reset the link state.
 *
 * @romaddress 0x08046c40
 */
void multiplayer_abortSession(void)
{
    if (!(REG_SIOCNT & 4))
    {
        REG_IE &= ~0x40;
    }
    REG_IE &= ~0x80;
    REG_SIOMLT_SEND = 0xffff;
    g_MpLinkState[4] = 0;
    g_MpLinkState[5] = 0xff;
}
