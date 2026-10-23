#define sfx_initDriver hdr_dummy
#include "sfx.h"
#include "variables.h"
#undef sfx_initDriver

/**
 * Initialize the sound driver and apply the saved volume settings.
 *
 * @romaddress 0x080470b0
 */
void sfx_initDriver(void)
{
    sfx_unknown0804aa80(1);
    sfx_unknown0804acf0(0x40080);
    sfx_unknown0804ac68(2);
    sfx_setMusicFlag(g_UnknownMusic_0300765c);
    sfx_setVolume(g_SaveGlobals.d.sfxVolume);
    sfx_setMusicVolume(g_SaveGlobals.d.musicVolume);
    g_CurrentSongId = 0xff;
    g_MusicPlaying = 0;
    sfx_unknown0804cefc(sfx_unknown08047374);
}
