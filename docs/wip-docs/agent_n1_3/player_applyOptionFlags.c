#include "game.h"
#include "multiplayer.h"
#include "player.h"
#include "save.h"
#include "variables.h"

/**
 * Copy the saved option flags into the player and sync them to the co-op partner.
 *
 * @romaddress 0x080265c0
 */
void player_applyOptionFlags(u32 playerIndex)
{
    volatile u32 mask;
    u8 flags;
    union { MpMessageArgs a; u8 b[2]; } args;

    mask = multiplayer_getMessageMask();
    flags = PLAYER(playerIndex).optionFlags.p;
    if (g_SaveGlobals.d.optionsFlags.p & OPTION_FLAG_AUTO_HEALING)
    {
        flags |= PLAYER_OPTION_FLAG_AUTO_HEALING;
    }
    else
    {
        flags &= ~PLAYER_OPTION_FLAG_AUTO_HEALING;
    }
    if (g_SaveGlobals.d.optionsFlags.p & OPTION_FLAG_QUICK_SKILL_SELECT)
    {
        flags |= PLAYER_OPTION_FLAG_QUICK_SKILL_SELECT;
    }
    else
    {
        flags &= ~PLAYER_OPTION_FLAG_QUICK_SKILL_SELECT;
    }
    if ((g_GameFlags.p & GAME_FLAG_COOP_ACTIVE) && (mask & 4))
    {
        args.b[0] = playerIndex;
        args.b[1] = flags;
        multiplayer_queueMessage(0x2d, args.a);
    }
    else
    {
        PLAYER(playerIndex).optionFlags.p = flags;
    }
}
