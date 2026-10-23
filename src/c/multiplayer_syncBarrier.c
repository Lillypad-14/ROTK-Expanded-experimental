#include "multiplayer.h"
#include "system.h"
#include "variables.h"

#define MASK (*(volatile u32 *)&g_MpMessageMask)

/**
 * Raise the barrier bit in the link message mask and wait until the peer's bit 2 arrives and
 * at least 20 frames passed, then lower the barrier bit.
 *
 * @romaddress 0x08046c94
 */
void multiplayer_syncBarrier(void)
{
    u32 frames = 0;

    MASK |= 1 << 17;
    (void)MASK;
    do
    {
        frames++;
        system_waitVBlank();
    } while (!(MASK & 4) || frames <= 19);
    MASK &= ~(1 << 17);
}
