#include "mission.h"
#include "sfx.h"
/**
 * Script op 10: set the music volume.
 *
 * @romaddress 0x080184ec
 */
void mission_script_op10(const u8 *command)
{
    sfx_setMusicVolume(command[4]);
}
