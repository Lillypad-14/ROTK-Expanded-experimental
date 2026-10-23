#include "eeprom.h"
#include "variables.h"

/**
 * Select the EEPROM device descriptor (4 or 64 Kbit); 0 ok, 1 unknown.
 *
 * @romaddress 0x0804dc74
 */
u32 eeprom_selectType(u16 typeCode)
{
    u32 result = 0;

    if (typeCode == 4)
    {
        g_EepromDescriptor = &EepromDesc512Byte;
    }
    else if (typeCode == 0x40)
    {
        g_EepromDescriptor = &EepromDesc8KByte;
    }
    else
    {
        g_EepromDescriptor = &EepromDesc512Byte;
        result = 1;
    }
    return result;
}
