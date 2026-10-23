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
    volatile u16 *cursor;
    u16 *output;
    u8 i;
    u8 j;

    if (address >= g_EepromDescriptor->maxBlock)
    {
        return 0x80ff;
    }
    cursor = bits + g_EepromDescriptor->addrBits + 1;
    for (i = 0; i < g_EepromDescriptor->addrBits; i++)
    {
        *cursor-- = address;
        address >>= 1;
    }
    *cursor-- = 1;
    *cursor = 1;
    eeprom_dmaBits(bits, EEPROM_SERIAL_PORT, g_EepromDescriptor->addrBits + 3);
    eeprom_dmaBits(EEPROM_SERIAL_PORT, bits, 68);
    cursor = bits + 4;
    output = (u16 *)dst + 3;
    i = 0;
    do
    {
        u16 value = 0;

        j = 0;
        do
        {
            value <<= 1;
            value |= *cursor++ & 1;
            j++;
        } while (j < 16);
        *output-- = value;
        i++;
    } while (i < 4);
    return 0;
}
