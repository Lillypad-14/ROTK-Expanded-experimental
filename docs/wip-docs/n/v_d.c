#include "sprite.h"
#include "variables.h"

void sprite_hideOamEntry(u32 index)
{
    s8 *entry = (s8 *)g_OamShadow + OAM_ENTRY_COUNT * 8;
    entry += index * 8;
    entry[1] = (entry[1] & -4) | 2;
}
