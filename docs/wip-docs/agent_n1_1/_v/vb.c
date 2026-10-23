#include "eeprom.h"
#include "variables.h"

/**
 * Read one block and compare it with expected.
 *
 * @return 0 on match, 0x8000 on mismatch, 0x80ff when blockIndex is out of range
 *
 * @romaddress 0x0804dff8
 */
u32 eeprom_verifyBlock(u16 blockIndex, void *expected)
{
    u16 buffer[4];
    u16 *read;
    u32 result = 0;
    u8 i;

    if (blockIndex < g_EepromDescriptor->maxBlock)
    {
        eeprom_readBlock(blockIndex, buffer);
        read = buffer;
        for (i = 0; i < 4; i++)
        {
            u16 a = *(u16 *)expected;
            u16 b = *read++;

            expected = (u16 *)expected + 1;
            if (a != b)
            {
                result = 0x8000;
                break;
            }
        }
    }
    else
    {
        result = 0x80ff;
    }
    return result;
}
