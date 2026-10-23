#include "game.h"
#include "gba_io.h"
#include "multiplayer.h"
#include "variables.h"

/**
 * End the co-op session: clear the connected flags, stop the link and reset the link state.
 *
 * @romaddress 0x08046cd4
 */
void multiplayer_endSession(void)
{
    g_MpMessageMask &= ~4;
    g_GameFlags.p &= ~GAME_FLAG_COOP_ACTIVE;
    *(s32 *)g_MpLinkState = -1;
    if (!(REG_SIOCNT & 4))
    {
        REG_IE &= ~0x40;
    }
    REG_IE &= ~0x80;
    REG_SIOMLT_SEND = 0xffff;
    g_MpLinkState[4] = 0;
    g_MpLinkState[5] = 0xff;
}
