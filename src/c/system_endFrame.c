#include "sprite.h"
#include "system.h"
#include "variables.h"

/**
 * Finish committed animation updates and release deferred OBJ tile allocations.
 *
 * @romaddress 0x08001768
 */
void system_endFrame(void)
{
    u32 i;

    if (g_EngineState->frameReady == 1)
    {
        for (i = 0; i < g_SpritePostCommitQueueCount; i++)
        {
            g_SpritePostCommitQueue[i]->flags.p &= ~ACTOR_FLAG_ANIMATION_FRAME_CHANGED;
        }
        g_SpritePostCommitQueueCount = 0;
        sprite_applyDeferredObjTileFrees();
    }
}
