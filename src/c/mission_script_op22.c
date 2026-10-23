#include "mission.h"
#include "sfx.h"
#include "variables.h"

/**
 * Script op 22: end the scripted camera pan on every active player and stop its sound.
 *
 * @romaddress 0x080185cc
 */
void mission_script_op22(const u8 *command)
{
    u8 i;

    (void)command;
    for (i = 0; i < g_PlayerGlobals.activePlayerCount; i++)
        g_MissionCameraPans[i].state = 0;
    if (sfx_isPlaying(g_UnknownSfxHandle_030035c4))
        sfx_stop(g_UnknownSfxHandle_030035c4);
}
