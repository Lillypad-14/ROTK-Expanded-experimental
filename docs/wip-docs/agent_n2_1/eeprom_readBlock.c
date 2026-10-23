#include "eeprom.h"
#include "variables.h"

/**
 * Read one 8-byte block into dst; the bitstream is one halfword per bit.
 *
 * @return 0 on success, 0x80ff when blockIndex is out of range
 *
 * @romaddress 0x0804de6c
 */
s32 eeprom_readBlock(u16 blockIndex, void *dst)
{
    u16 buffer[68];
    u16 *p;
    u16 *out;
    u8 i;
    u8 j;
    u32 idx = blockIndex;

    if (blockIndex < g_EepromDescriptor->maxBlock)
    {
    p = &buffer[g_EepromDescriptor->addrBits + 1];
    for (i = 0; i < g_EepromDescriptor->addrBits; i++)
    {
        *p-- = idx;
        idx >>= 1;
    }
    *p-- = 1;
    *p = 1;
    eeprom_dmaBits(buffer, (void *)0x0d000000, g_EepromDescriptor->addrBits + 3);
    eeprom_dmaBits((void *)0x0d000000, buffer, 0x44);
    p = &buffer[4];
    out = (u16 *)dst + 3;
    for (i = 0; i < 4; i++)
    {
        u16 acc = 0;

        for (j = 0; j < 16; j++)
        {
            acc = (acc << 1) | (*p++ & 1);
        }
        *out-- = acc;
    }
    return 0;
    }
    return 0x80ff;
}
