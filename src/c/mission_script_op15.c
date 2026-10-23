#include "game.h"
#include "mission.h"
#include "variables.h"

/**
 * Script op 15: set the mission variant, if it is not restricted to co-op.
 *
 * @romaddress 0x08018558
 */
void mission_script_op15(const u8 *command)
{
    if (!command[5] || (g_GameFlags.p & GAME_FLAG_COOP_ACTIVE))
        g_MissionVariant = command[4];
}
