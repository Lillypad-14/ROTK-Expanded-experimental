#include "sprite.h"
#include "variables.h"
s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    if ((s8)g_SpriteAllocCount < 0) { return -1; }
    g_SpriteAllocCount++;
    {
        u8 *base = g_OamShadow;
        OamEntryWords *a = (OamEntryWords *)(base + (index << 3));
        OamEntryWords *b;
        *a = *entry;
        base += 0x400;
        b = (OamEntryWords *)(base + (index << 3));
        *b = *entry;
    }
    return 0;
}
