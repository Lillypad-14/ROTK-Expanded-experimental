#include "sprite.h"
#include "variables.h"

void sprite_hideOamEntry(u32 index)
{
    OamEntryWords *second = (OamEntryWords *)g_OamShadow + OAM_ENTRY_COUNT;
    second[index].entry.affineMode = 2;
}
