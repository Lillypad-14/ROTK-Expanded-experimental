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
    s32 priority;

    for (priority = 0; priority < 4; priority++)
    {
        u32 count = 0;
        FxParticle *particle = (FxParticle *)g_FxParticleList.head;

        if (particle != NULL)
        {
            u32 queuedMask = FX_PARTICLE_FLAG_RENDER_QUEUED;
            u32 earlierMask = FX_PARTICLE_FLAG_QUEUE_EARLIER;
            MATCH_PIN(FxParticle ***, base, "r1") = g_FxRenderQueues;
            MATCH_PIN(FxParticle ***, address, "r0");
            MATCH_PIN(FxParticle ***, slot, "r12");

            MATCH_FRESH(address, (FxParticle ***)((u32)priority * sizeof(FxParticle **) + (u32)base));
            slot = address;

            do
            {
            FxParticle *next = particle->next;
            MATCH_PIN(u32, flags, "r3") = particle->flags;

            if (!(flags & queuedMask) &&
                (particle->oam.bgPriority == priority ||
                 ((flags & earlierMask) && particle->oam.bgPriority - 1 == priority)))
            {
                MATCH_PIN(FxParticle **, queue, "r0") = *slot;

                queue += count;
                *queue = particle;
                count++;
                particle->flags |= queuedMask;
            }
            particle = next;
            } while (particle != NULL);
        }
        g_FxRenderQueueCounts[priority] = count;
    }
}
