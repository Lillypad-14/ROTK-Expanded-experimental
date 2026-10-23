#include "sfx.h"
#include "variables.h"

bool32 sfx_driverIsMusicPlaying(void)
{
    return g_SfxMusic.state == 1 ? TRUE : FALSE;
}
