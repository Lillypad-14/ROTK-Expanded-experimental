#include "sfx.h"
#include "variables.h"

bool32 sfx_driverIsMusicPlaying(void)
{
    SfxMusicDriver *music = &g_SfxMusic;
    bool32 playing = 0;

    if (music->state == 1)
    {
        playing = 1;
    }
    return playing;
}
