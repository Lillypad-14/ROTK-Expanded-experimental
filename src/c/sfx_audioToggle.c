#include "sfx.h"
#include "variables.h"

/**
 * Silence the sound engine.
 *
 * @romaddress 0x0804711c
 */
void sfx_disableAudio(void)
{
    sfx_libDisable();
}

/**
 * Restart the sound engine.
 *
 * @romaddress 0x08047128
 */
void sfx_enableAudio(void)
{
    sfx_libEnable();
}

/**
 * Store the music state flag.
 *
 * @romaddress 0x08047134
 */
void sfx_setMusicFlag(u8 value)
{
    g_UnknownMusic_0300765c = value;
}
