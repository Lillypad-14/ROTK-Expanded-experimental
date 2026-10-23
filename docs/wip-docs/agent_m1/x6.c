#include "gba_io.h"
#include "sfx.h"
extern u32 g_SfxAudioActive;

extern vu16 g_a;
void sfx_libEnable(void)
{
    REG_IE |= 0x10;
    REG_TM1CNT_H = 0x80;
    g_SfxAudioActive = 1;
    return;
}
