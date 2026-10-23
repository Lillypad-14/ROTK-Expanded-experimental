#include "mission.h"

/**
 * Get the word-aligned byte size of a halfword sequence, with 16-bit wrapping.
 *
 * @romaddress 0x0800c7d8
 */
u16 mission_script_getAlignedHalfwordBytes(u16 count)
{
    if (count & 1)
    {
        count++;
    }
    count *= 2;
    return count;
}

/**
 * Round a byte size up to a word boundary, with 16-bit wrapping.
 *
 * @romaddress 0x0800c7f8
 */
u16 mission_script_alignByteSize(u16 size)
{
    u32 padding = size & 3;

    if (padding == 1)
    {
        padding = 3;
    }
    else if (padding == 2)
    {
    }
    else if (padding == 3)
    {
        padding = 1;
    }
    return size + padding;
}
