#include "sfx.h"
#include "variables.h"

bool32 sfx_driverIsMusicPlaying(void)
{
    bool32 playing = FALSE;
    SfxMusicDriver *music = &g_SfxMusic;

    if (music->state == 1)
    {
        playing = TRUE;
    }
    return playing;
}
