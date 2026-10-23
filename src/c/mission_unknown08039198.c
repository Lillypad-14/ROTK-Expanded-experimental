#include "actor.h"
#include "fx.h"
#include "variables.h"

/**
 * Free every live particle.
 *
 * @romaddress 0x08039198
 */
void mission_unknown08039198(void)
{
    Actor *node;
    Actor *next;

    if (g_FxParticleCount != 0)
    {
        node = g_FxParticleList.head;
        if (node != NULL)
        {
            do
            {
                next = *(Actor **)node;
                fx_freeParticle(node);
                node = next;
            } while (node != NULL);
        }
    }
}
