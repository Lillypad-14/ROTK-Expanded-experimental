#include "actor.h"
#include "text.h"
#include "variables.h"

/**
 * Create a text object from *pstr and store it in slot slotIndex of g_TextObjSlots.
 *
 * @return the created text object
 *
 * @romaddress 0x08047d10
 */
void *text_objCreateInSlot(char **pstr, s32 slotIndex)
{
    Actor *obj = text_objFromString(pstr);

    g_TextObjSlots[slotIndex] = obj;
    *(u16 *)((u8 *)obj + 0xf0) = slotIndex;
    return obj;
}

/**
 * Create one text object per line of *pstr into consecutive slots from firstSlot.
 *
 * @return the number of objects created
 *
 * @romaddress 0x08047d30
 */
u8 text_objCreateAllInSlots(char **pstr, s32 firstSlot)
{
    u8 count = 0;
    Actor **base = g_TextObjSlots;
    Actor **slot = base + firstSlot;

    do
    {
        Actor *obj = text_objFromString(pstr);

        *slot++ = obj;
        *(u16 *)((u8 *)obj + 0xf0) = firstSlot;
        count++;
        firstSlot++;
    } while (**pstr != 0);
    return count;
}
