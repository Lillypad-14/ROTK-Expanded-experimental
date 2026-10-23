#include "mission.h"
#include "sfx.h"
#include "variables.h"

/**
 * Script op 21: start a scripted camera pan on every active player and play its sound.
 *
 * @romaddress 0x08017840
 */
void mission_script_op21(const u8 *command)
{
    u8 i;

    for (i = 0; i < g_PlayerGlobals.activePlayerCount; i++)
    {
        g_MissionCameraPans[i].speed = *(const s16 *)(command + 6) << 16;
        g_MissionCameraPans[i].frames = *(const u16 *)(command + 4) * 30;
        g_MissionCameraPans[i].state = 3;
        g_MissionCameraPans[i].duration = 0;
        if (i == 0)
        {
            g_MissionCameraPans[0].destX = command[8];
            g_MissionCameraPans[0].destY = command[9];
        }
        else
        {
            g_MissionCameraPans[i].destX = 0;
            g_MissionCameraPans[i].destY = 0;
        }
        if (g_MissionCameraPans[i].frames == 0)
            g_MissionCameraPans[i].field_0x20 = 1;
        else
            g_MissionCameraPans[i].field_0x20 = 0;
        g_MissionCameraPans[i].field_0x28 = 1;
    }
    if (!sfx_isPlaying(g_UnknownSfxHandle_030035c4))
        g_UnknownSfxHandle_030035c4 = sfx_play(0x10e);
}
