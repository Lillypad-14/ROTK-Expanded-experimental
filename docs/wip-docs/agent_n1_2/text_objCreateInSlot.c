#define text_objCreateInSlot hdr_dummy
#include "actor.h"
#include "text.h"
#include "variables.h"
#undef text_objCreateInSlot

/**
 * Create a text object from *pstr and store it in slot slotIndex of g_TextObjSlots.
 *
 * @return the created text object
 *
 * @romaddress 0x08047d10
 */
Actor *text_objCreateInSlot(char **pstr, s32 slotIndex)
{
    Actor *obj = text_objFromString(pstr);

    g_TextObjSlots[slotIndex] = obj;
    *(u16 *)((u8 *)obj + 0xf0) = slotIndex;
    return obj;
}

// Header change: text_objCreateInSlot returns the object (Actor *, or void *).

// Unnamed 0x08047d30 (52 bytes, follows the function above): create one text object per line
// of *pstr into consecutive slots from firstSlot; returns the number created.
// Needs a config name (e.g. text_objCreateAllInSlots), prototype u8 (char **pstr, s32 firstSlot).
u8 text_objCreateAllInSlots(char **pstr, s32 firstSlot)
{
    u8 count = 0;
    Actor **slot = &g_TextObjSlots[firstSlot];

    do {
        Actor *obj = text_objFromString(pstr);

        *slot++ = obj;
        *(u16 *)((u8 *)obj + 0xf0) = firstSlot;
        count++;
        firstSlot++;
    } while (**pstr != 0);
    return count;
}
