#include "sprite.h"
#include "variables.h"
s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    if ((s8)g_SpriteAllocCount < 0) { return -1; }
    g_SpriteAllocCount++;
    {
        OamEntryWords *a = (OamEntryWords *)g_OamShadow;
        a[index] = *entry;
        a = (OamEntryWords *)((u8 *)a + 0x400);
        a[index] = *entry;
    }
    return 0;
}
