#include "memory.h"
#include "player.h"
#include "save.h"
#include "variables.h"

/**
 * Zero stats[] and the per-mission scratch, apply the base stats, then reload the slot state.
 *
 * @romaddress 0x08025ee0
 */
void player_rebuildStats(u8 playerIndex)
{
    memory_memset(PLAYER(playerIndex).stats, 0, sizeof(PLAYER(playerIndex).stats));
    PLAYER(playerIndex).activeSkillStateFlags = 0;
    PLAYER(playerIndex).bowHoldFrames = 0;
    PLAYER(playerIndex).field_0x1a4 = 0;
    PLAYER(playerIndex).shieldBubble = NULL;
    PLAYER(playerIndex).ringActor = NULL;
    PLAYER(playerIndex).aimFxActor = NULL;
    PLAYER(playerIndex).combatFlags.p &= 0x0dff;
    player_applyBaseStats(playerIndex);
    save_loadSlotState(playerIndex);
    g_UnknownRebuildFlag_03004210 = 0;
}
