#include "sprite.h"
#include "system.h"
#include "variables.h"

/**
 * Hide unused OAM entries in both shadow pages and mark the frame ready to commit.
 *
 * @romaddress 0x080366a4
 */
void sprite_endFrame(void)
{
    OamEntryWords *shadow = (OamEntryWords *)g_OamShadow;
    OamEntryWords *second = shadow + OAM_ENTRY_COUNT;
    u32 i = g_SpriteAllocCount;

    for (; i < OAM_ENTRY_COUNT; i++)
    {
        shadow[i].words[0] = OAM_AFFINE_HIDDEN << 8;
        second[i].words[0] = OAM_AFFINE_HIDDEN << 8;
    }
    g_EngineState->frameReady = 1;
}
