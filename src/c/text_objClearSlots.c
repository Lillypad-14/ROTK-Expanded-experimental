#include "text.h"
#include "variables.h"

/**
 * Forget every text object slot.
 *
 * @romaddress 0x08047d94
 */
void text_objClearSlots(void)
{
    u8 i;
    for (i = 0; i < 6; i++)
    {
        g_TextObjSlots[i] = NULL;
    }
}
