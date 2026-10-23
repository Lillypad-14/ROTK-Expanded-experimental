#include "mission.h"
#include "player.h"
#include "sfx.h"
#include "variables.h"

/**
 * Script op 27: grant attribute points to every active player and flag them as unspent.
 *
 * @romaddress 0x08017f68
 */
void mission_script_op27(const u8 *command)
{
    u8 i;

    for (i = 0; i < g_PlayerGlobals.activePlayerCount; i++)
    {
        PLAYER(i).attributePoints += command[6];
        PLAYER(i).statusFlags.p |= PLAYER_STATUS_FLAG_UNSPENT_POINTS;
        if (g_PlayerHuds[i].skillPanelState == PLAYERHUD_SKILL_PANEL_IDLE)
            g_PlayerHuds[i].skillPanelState = PLAYERHUD_SKILL_PANEL_DRAW_UNSPENT_POINTS;
        sfx_play(0x11f);
    }
}
