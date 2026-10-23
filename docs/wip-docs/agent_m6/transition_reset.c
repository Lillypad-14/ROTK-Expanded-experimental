#include "game.h"
#include "gfx.h"
#include "save.h"
#include "scene.h"
#include "sfx.h"
#include "variables.h"

/**
 * Reset g_TransitionState, stop the jingle, and restore the saved music volume.
 *
 * @romaddress 0x0804a424
 */
void transition_reset(void)
{
    g_TransitionState.field_0x09[3] = 0;
    g_TransitionState.field_0x09[2] = 0;
    g_TransitionState.field_0x09[0] = 0;
    g_TransitionState.effect = 0;
    g_TransitionState.field_0x0e = 0;
    g_TransitionState.field_0x00 = 0;
    g_TransitionState.flags = 0;
    if ((g_GameFlags.p & (GAME_FLAG_OVERLAY | GAME_FLAG_COOP_ACTIVE)) != (GAME_FLAG_OVERLAY | GAME_FLAG_COOP_ACTIVE) &&
        (g_GameFlags.p & (1 << 25)) == 0)
    {
        gfx_disableBg(1);
    }
    if (sfx_isPlaying(g_TransitionState.sfxHandle))
    {
        sfx_stop(g_TransitionState.sfxHandle);
    }
    sfx_setMusicVolume(g_SaveGlobals.d.musicVolume);
}
