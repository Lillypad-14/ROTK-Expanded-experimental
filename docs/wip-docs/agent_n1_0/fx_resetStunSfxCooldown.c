#include "fx.h"
#include "variables.h"

/**
 * Reset the stun sound effect cooldown.
 *
 * @romaddress 0x0801c514
 */
void fx_resetStunSfxCooldown(void)
{
    g_StunSfxCooldown = 0;
}
