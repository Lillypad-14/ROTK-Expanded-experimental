#include "fx.h"
#include "match_hacks.h"
#include "variables.h"

/**
 * Partition live particles into render queues by OAM priority.
 *
 * @romaddress 0x08038728
 */
void fx_buildRenderQueues(void)
{
    MATCH_PIN(u8 *, counts, "sl");
    s32 priority;

    priority = 0;
    counts = g_FxRenderQueueCounts;
    for (; priority < 4; priority++)
    {
        u32 count = 0;
        FxParticle *particle = (FxParticle *)g_FxParticleList.head;

        while (particle != NULL)
        {
            FxParticle *next = particle->next;
            u32 flags = particle->flags;
            u32 queuedMask = FX_PARTICLE_FLAG_RENDER_QUEUED;
            u32 earlierMask = FX_PARTICLE_FLAG_QUEUE_EARLIER;

            if (!(flags & queuedMask) &&
                (particle->oam.bgPriority == priority ||
                 ((flags & earlierMask) && particle->oam.bgPriority - 1 == priority)))
            {
                g_FxRenderQueues[priority][count++] = particle;
                particle->flags |= queuedMask;
            }
            particle = next;
        }
        counts[priority] = count;
    }
}
