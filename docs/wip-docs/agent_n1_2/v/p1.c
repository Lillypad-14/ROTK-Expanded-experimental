#include "sprite.h"
#include "variables.h"

s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    if ((s8)g_SpriteAllocCount < 0)
    {
        return -1;
    }
    g_SpriteAllocCount++;
    {
        u8 *shadow = g_OamShadow;
        u32 off = index << 3;
        *(OamEntryWords *)(shadow + off) = *entry;
        *(OamEntryWords *)(shadow + 0x400 + off) = *entry;
    }
    return 0;
}
