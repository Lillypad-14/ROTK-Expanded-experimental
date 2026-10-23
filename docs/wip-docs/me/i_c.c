#include "sfx.h"
#include "variables.h"

bool32 sfx_driverIsMusicPlaying(void)
{
    u32 state = g_SfxMusic.state;
    bool32 playing = FALSE;

    if (state == 1)
    {
        playing = TRUE;
    }
    return playing;
}
