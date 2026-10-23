#include "actor.h"
#include "mission.h"
#include "multiplayer.h"
#include "save.h"
#include "sfx.h"
#include "variables.h"

/**
 * Script op 45: on a received link message, set one of 16 unlock flags, then save the globals
 * with audio disabled.
 *
 * @romaddress 0x08018098
 */
void mission_script_op45(const u8 *command)
{
    volatile u32 mask = multiplayer_getMessageMask();

    if (mask & 4)
    {
        switch (command[4])
        {
        case 0:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 4;
            break;
        case 1:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 5;
            break;
        case 2:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 6;
            break;
        case 3:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 7;
            break;
        case 4:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 20;
            break;
        case 5:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 21;
            break;
        case 6:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 22;
            break;
        case 7:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 23;
            break;
        case 8:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 24;
            break;
        case 9:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 25;
            break;
        case 10:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 26;
            break;
        case 11:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 27;
            break;
        case 12:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 16;
            break;
        case 13:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 17;
            break;
        case 14:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 18;
            break;
        case 15:
            g_SaveGlobals.d.unlockFlags.p |= 1 << 19;
            break;
        }
        sfx_disableAudio();
        save_writeGlobalHeader();
        sfx_enableAudio();
    }
}
