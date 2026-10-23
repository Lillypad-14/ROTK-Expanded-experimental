#include "mission.h"
#include "player.h"
#include "sfx.h"

/**
 * Script op 29: award experience and play the reward sound.
 *
 * @romaddress 0x080186d4
 */
void mission_script_op29(const u8 *command)
{
    player_awardXp(*(const s16 *)(command + 6));
    sfx_play(0x11f);
}
