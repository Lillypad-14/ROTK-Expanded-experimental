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
        *(OamEntryWords *)(shadow + (index << 3)) = *entry;
        *(OamEntryWords *)(shadow + 0x400 + (index << 3)) = *entry;
    }
    return 0;
}
