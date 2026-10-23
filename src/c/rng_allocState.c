#include "memory.h"
#include "variables.h"

/**
 * Allocate the MT19937 state buffer.
 *
 * @romaddress 0x0803a46c
 */
void rng_allocState(void)
{
    g_RngMtState = memory_malloc(0x9c4);
}
