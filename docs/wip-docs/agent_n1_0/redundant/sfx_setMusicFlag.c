#include "sfx.h"
#include "variables.h"

/**
 * @romaddress 0x08047134
 */
void sfx_setMusicFlag(u8 value)
{
    g_UnknownMusic_0300765c = value;
}
