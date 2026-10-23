#include "multiplayer.h"
#include "variables.h"

/**
 * SIOCNT multi-player ID of this GBA (0 = parent/host).
 *
 * @romaddress 0x08046d64
 */
s32 multiplayer_getMultiPlayerId(void)
{
    return (s8)g_MpLinkState[5];
}

/**
 * Set the scene the multiplayer abort path returns to.
 *
 * @romaddress 0x08046d74
 */
void multiplayer_setAbortScene(u32 sceneId)
{
    g_MpAbortScene = sceneId;
}
