#include "gba_io.h"
#include "sfx.h"
extern u32 g_SfxAudioActive;

static const u32 k10 = 0x10;
static const u32 k80 = 0x80;
void sfx_libEnable(void)
{
    REG_IE |= k10;
    REG_TM1CNT_H = k80;
    g_SfxAudioActive = 1;
}
