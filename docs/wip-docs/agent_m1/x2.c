#include "gba_io.h"
#include "sfx.h"
extern u32 g_SfxAudioActive;

void sfx_libEnable(void)
{
    vu16 *ie = (vu16 *)0x04000200;
    vu16 *tm = (vu16 *)0x04000102;
    u32 a = 0x10;
    u32 b = 0x80;
    *ie = *ie | a;
    *tm = b;
    g_SfxAudioActive = 1;
}
