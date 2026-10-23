#include "player.h"
#include "variables.h"

/**
 * Clear the first word after the text buffer of both player HUDs.
 *
 * @romaddress 0x08029804
 */
void player_resetHuds(void)
{
    u8 i;

    for (i = 0; i < 2; i++)
        *(u32 *)g_PlayerHuds[i].field_0x6c = 0;
}
