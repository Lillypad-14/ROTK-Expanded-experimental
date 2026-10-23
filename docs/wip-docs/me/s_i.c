#include "sfx.h"
#include "variables.h"

typedef struct Slot { u8 a; u8 b; u8 status; u8 pad[0x21]; u8 active; u8 pad2[7]; } Slot;

void sfx_driverStopAll(void)
{
    Slot *s = (Slot *)g_SfxSlots;
    s32 i = 32;

    do
    {
        if (s->active != 0)
        {
            if (s->status == 1)
            {
                s->status = 0;
            }
        }
        s++;
    } while (--i != 0);
}
