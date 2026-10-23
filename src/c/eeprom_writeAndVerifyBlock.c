
#include "eeprom.h"


/**
 * Write a block, retrying up to three times, then verify it.
 *
 * @return 0 on success
 *
 * @romaddress 0x0804e050
 */
u32 eeprom_writeAndVerifyBlock(u16 blockIndex, void *src)
{
    u8 i;
    u16 result = result;

    for (i = 0; i < 3; i++)
    {
        result = eeprom_writeBlock(blockIndex, src);
        if (result != 0)
        {
            continue;
        }
        result = eeprom_verifyBlock(blockIndex, src);
        if (result == 0)
        {
            break;
        }
    }
    return result;
}
