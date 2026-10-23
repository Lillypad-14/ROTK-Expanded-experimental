#include "sprite.h"
#include "variables.h"

/**
 * Set an entry of the second OAM shadow page to the hidden affine mode.
 *
 * @romaddress 0x08036590
 */
void sprite_hideOamEntry(u32 index)
{
    s8 *entry = (s8 *)g_OamShadow + OAM_ENTRY_COUNT * 8;
    entry += index * 8;
    entry[1] = (entry[1] & -4) | 2;
}
