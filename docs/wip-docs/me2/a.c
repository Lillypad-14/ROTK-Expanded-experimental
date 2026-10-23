#include "actor.h"
#include "text.h"
#include "variables.h"
/** @romaddress 0x08047d30 */
u8 text_objCreateAllInSlots(char **pstr, s32 firstSlot)
{
    u8 count = 0;
    Actor **base = g_TextObjSlots;
    Actor **slot = base + firstSlot;
    do { Actor *obj = text_objFromString(pstr); *slot++ = obj; *(u16 *)((u8 *)obj + 0xf0) = firstSlot; count++; firstSlot++; } while (**pstr != 0);
    return count;
}
