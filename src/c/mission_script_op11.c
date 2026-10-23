#include "mission.h"
#include "sfx.h"
/**
 * Script op 11: set the sound effect volume.
 *
 * @romaddress 0x080184f8
 */
void mission_script_op11(const u8 *command)
{
    sfx_setVolume(command[4]);
}
