#include "mission.h"
#include "sfx.h"
/**
 * Script op 9: play a sound effect.
 *
 * @romaddress 0x080184e0
 */
void mission_script_op09(const u8 *command)
{
    sfx_play(command[4]);
}
