#include "actor.h"
#include "fx.h"
#include "variables.h"

/**
 * Return a particle to the particle pool.
 *
 * @romaddress 0x08039300
 */
void fx_freeParticle(Actor *particle)
{
    pool_free(&g_FxParticleFreeList, &g_FxParticleList, particle);
    g_FxParticleCount--;
}
