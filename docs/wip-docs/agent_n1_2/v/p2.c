#include "sprite.h"
#include "variables.h"
s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    if ((s8)g_SpriteAllocCount < 0) { return -1; }
    g_SpriteAllocCount++;
    {
        OamEntryWords *a = (OamEntryWords *)g_OamShadow + index;
        OamEntryWords *b = (OamEntryWords *)(g_OamShadow + 0x400) + index;
        *a = *entry;
        *b = *entry;
    }
    return 0;
}
