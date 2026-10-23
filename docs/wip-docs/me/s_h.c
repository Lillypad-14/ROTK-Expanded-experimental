#include "sfx.h"
#include "variables.h"

typedef struct Slot { u8 a; u8 b; u8 status; u8 pad[0x21]; u8 active; u8 pad2[7]; } Slot;

void sfx_driverStopAll(void)
{
    Slot *s = (Slot *)g_SfxSlots;
    u8 i;
    u32 zero = 0;

    for (i = 0; i < 32; i++)
    {
        if (s->active && s->status == 1)
        {
            s->status = zero;
        }
        s++;
    }
}
