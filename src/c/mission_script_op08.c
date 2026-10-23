#include "game.h"
#include "mission.h"
#include "sfx.h"
#include "variables.h"

/**
 * Script op 8: request a song, flagging the game when the current stream's slot is loaded.
 *
 * @romaddress 0x080184ac
 */
void mission_script_op08(const u8 *command)
{
    if (!mission_script_slotNeedsLoad(g_MissionCurrentStream))
        g_GameFlags.p |= 1 << 24;
    sfx_requestSong(command[4]);
}
