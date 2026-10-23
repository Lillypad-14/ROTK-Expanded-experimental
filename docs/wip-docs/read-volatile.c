#include "eeprom.h"
#include "variables.h"

#define EEPROM_SERIAL_PORT ((void *)0x0d000000)

/**
 * Read an EEPROM block and decode its serial response into four halfwords.
 *
 * @return 0 on success, 0x80ff when blockIndex is out of range.
 *
 * @romaddress 0x0804de6c
 */
s32 eeprom_readBlock(u16 blockIndex, void *dst)
{
    u16 bits[68];
    u32 address = blockIndex;
    u16 *cursor;
    u8 i;
    u8 j;

    if (address >= (*(const EepromDesc *volatile *)&g_EepromDescriptor)->maxBlock)
    {
        return 0x80ff;
    }
    cursor = bits + (*(const EepromDesc *volatile *)&g_EepromDescriptor)->addrBits + 1;
    for (i = 0; i < (*(const EepromDesc *volatile *)&g_EepromDescriptor)->addrBits; i++)
    {
        *cursor-- = address;
        address >>= 1;
    }
    *cursor-- = 1;
    *cursor = 1;
    eeprom_dmaBits(bits, EEPROM_SERIAL_PORT, (*(const EepromDesc *volatile *)&g_EepromDescriptor)->addrBits + 3);
    eeprom_dmaBits(EEPROM_SERIAL_PORT, bits, 68);
    cursor = bits + 4;
    dst = (u16 *)dst + 3;
    for (i = 0; i < 4; i++)
    {
        u16 value = 0;

        for (j = 0; j < 16; j++)
        {
            value <<= 1;
            value |= *cursor++ & 1;
        }
        *(u16 *)dst = value;
        dst = (u16 *)dst - 1;
    }
    return 0;
}
