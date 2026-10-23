#include "sprite.h"
#include "variables.h"

s32 sprite_pushOam(u8 index, OamEntryWords *entry)
{
    u8 *shadow;

    if ((s8)g_SpriteAllocCount < 0)
    {
        return -1;
    }
    g_SpriteAllocCount++;
    shadow = g_OamShadow;
    *(OamEntryWords *)(shadow + index * 8) = *entry;
    *(OamEntryWords *)(shadow + OAM_ENTRY_COUNT * 8 + index * 8) = *entry;
    return 0;
}
