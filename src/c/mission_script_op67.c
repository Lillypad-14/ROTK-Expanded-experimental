#include "mission.h"
#include "sfx.h"

/**
 * Script op 67: play a song table entry.
 *
 * @romaddress 0x08018a88
 */
void mission_script_op67(const u8 *command)
{
    sfx_playSongEntry(command[4]);
}
