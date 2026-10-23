#include "sfx.h"
#include "variables.h"

void sfx_driverStopAll(void)
{
    s32 i = 32;
    u8 *slot = g_SfxSlots;
    u8 zero = 0;

    do
    {
        if (slot[0x24] != 0 && slot[2] == 1)
        {
            slot[2] = zero;
        }
        i--;
        slot += 0x2c;
    } while (i != 0);
}
