#include "sprite.h"
#include "variables.h"

s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    u8 *shadow;
    u32 off;
    u8 count = g_SpriteAllocCount;

    if ((s8)g_SpriteAllocCount < 0)
    {
        return -1;
    }
    g_SpriteAllocCount = count + 1;
    shadow = g_OamShadow;
    off = index << 3;
    *(OamEntryWords *)(shadow + off) = *entry;
    *(OamEntryWords *)(shadow + OAM_ENTRY_COUNT * 8 + off) = *entry;
    return 0;
}
