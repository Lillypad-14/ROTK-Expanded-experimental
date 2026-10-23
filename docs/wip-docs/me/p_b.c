#include "sprite.h"
#include "variables.h"

s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    u8 *shadow;
    u8 count = g_SpriteAllocCount;

    if ((s8)g_SpriteAllocCount < 0)
    {
        return -1;
    }
    g_SpriteAllocCount = count + 1;
    shadow = g_OamShadow;
    *(OamEntryWords *)(shadow + index * 8) = *entry;
    shadow += OAM_ENTRY_COUNT * 8;
    *(OamEntryWords *)(shadow + index * 8) = *entry;
    return 0;
}
