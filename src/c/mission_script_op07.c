#include "mission.h"
#include "sfx.h"
/**
 * Script op 7: pause the music.
 *
 * @romaddress 0x080184a0
 */
void mission_script_op07(const u8 *command)
{
    (void)command;
    sfx_pauseMusic();
}
