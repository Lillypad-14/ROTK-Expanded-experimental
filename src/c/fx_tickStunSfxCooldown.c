#include "fx.h"
#include "variables.h"

/**
 * Count the stun sound cooldown down toward zero.
 *
 * @romaddress 0x0801c520
 */
void fx_tickStunSfxCooldown(void)
{
    if (g_StunSfxCooldown > 0)
    {
        g_StunSfxCooldown--;
    }
}
