#define text_objClearSlots hdr_dummy
#include "text.h"
#include "variables.h"
#undef text_objClearSlots

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
