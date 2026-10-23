#include "mission.h"
#include "sfx.h"
/**
 * Script op 6: resume the paused music.
 *
 * @romaddress 0x08018494
 */
void mission_script_op06(const u8 *command)
{
    (void)command;
    sfx_resumeMusic();
}
