#define input_disable hdr_dummy
#include "input.h"
#include "variables.h"
#undef input_disable

/**
 * Disable input for all players.
 *
 * @romaddress 0x08029dd8
 */
void input_disable(void)
{
    u16 *base = g_InputDisableFlags;
    u16 *p = base + 1;
    do
    {
        *p = 1;
        p--;
    } while ((s32)p >= (s32)base);
}
