#include "rng.h"
#include "system.h"
#include "variables.h"

/**
 * Reseed both random streams from the frame counter, the held keys and a reseed counter.
 *
 * @romaddress 0x0803a280
 */
void rng_reseed(void)
{
    g_RngGameplayRemaining = -1;
    g_RngMtRemaining = -1;
    g_RngSeed += (g_EngineState->vblankCounter << 4) + (g_PlayerKeysCurrent[0] << 2);
    if (g_RngSeed == 0)
    {
        g_RngSeed = rng_nextGameplay() % 0x000fff1f + 1;
    }
    g_RngReseedCount++;
    g_RngSeed = ((g_RngSeed << 8) + (g_EngineState->vblankCounter << 6) + (g_RngReseedCount << 4)) |
                g_PlayerKeysCurrent[0];
    rng_seedState(g_RngSeed);
    g_RngGameplayLastByte = 0;
    g_Unknown030054f4 = 0;
}
