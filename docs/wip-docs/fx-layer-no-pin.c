#include "fx.h"
#include "variables.h"

/**
 * Tick and render one particle queue, returning its final render status.
 *
 * @romaddress 0x08039268
 */
s32 fx_renderLayer(u8 priority)
{
    s32 result = 0;
    s32 i;

    if (g_FxRenderQueueCounts[priority] == 0)
    {
        return 0;
    }
    for (i = g_FxRenderQueueCounts[priority] - 1; i >= 0; i--)
    {
        FxParticle **queue = g_FxRenderQueues[priority];
        FxParticle *particle;

        queue += i;
        particle = *queue;

        result = (s8)fx_tickAndRenderParticle(particle);
        particle->flags &= ~FX_PARTICLE_FLAG_RENDER_QUEUED;
    }
    return result;
}
