#include "mission.h"
#include "player.h"
#include "variables.h"

/**
 * Script op 54: hide the boss HP bar.
 *
 * @romaddress 0x0801894c
 */
void mission_script_op54(const u8 *command)
{
    (void)command;
    g_PlayerHuds[0].flags.p &= ~PLAYERHUD_FLAG_BOSS_BAR;
}
