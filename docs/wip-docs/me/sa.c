#include "sfx.h"
#include "variables.h"

void sfx_driverStopAll(void)
{
    u8 *slot = g_SfxSlots;
    s32 i = 32;

    do
    {
        if (slot[0x24] != 0 && slot[2] == 1)
        {
            slot[2] = 0;
        }
        i--;
        slot += 0x2c;
    } while (i != 0);
}
