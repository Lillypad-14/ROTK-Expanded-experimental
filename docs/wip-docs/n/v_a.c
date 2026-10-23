#include "sprite.h"
#include "variables.h"

void sprite_hideOamEntry(u32 index)
{
    u8 *entry = g_OamShadow + OAM_ENTRY_COUNT * 8 + index * 8;
    entry[1] = (entry[1] & ~3) | 2;
}
