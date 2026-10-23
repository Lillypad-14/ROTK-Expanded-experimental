#include "gba_io.h"
#include "sfx.h"
extern u32 g_SfxAudioActive;

void sfx_libEnable(void)
{
    u32 v;
    v = REG_IE;
    v |= 0x10;
    REG_IE = v;
    REG_TM1CNT_H = 0x80;
    g_SfxAudioActive = 1;
}
