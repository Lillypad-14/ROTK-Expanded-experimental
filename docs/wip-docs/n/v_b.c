#include "sprite.h"
#include "variables.h"

void sprite_hideOamEntry(u32 index)
{
    OamEntryWords *entry = (OamEntryWords *)(g_OamShadow + OAM_ENTRY_COUNT * 8) + index;
    entry->entry.affineMode = 2;
}
