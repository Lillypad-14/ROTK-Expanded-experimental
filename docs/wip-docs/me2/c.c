#include "actor.h"
#include "text.h"
#include "variables.h"
/** @romaddress 0x08047d30 */
u8 text_objCreateAllInSlots(char **pstr, s32 firstSlot)
{
    u8 count = 0;
    u32 off = firstSlot << 2;
    Actor **slot = (Actor **)(off + (u32)g_TextObjSlots);
    do { Actor *obj = text_objFromString(pstr); *slot++ = obj; *(u16 *)((u8 *)obj + 0xf0) = firstSlot; count++; firstSlot++; } while (**pstr != 0);
    return count;
}
