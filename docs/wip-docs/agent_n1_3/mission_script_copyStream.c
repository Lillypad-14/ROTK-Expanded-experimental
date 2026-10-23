#include "mission.h"
#include "variables.h"

/**
 * Copy one opcode stream out of a mission script section, up to and including its terminator.
 *
 * @param scriptSection section whose u16 offset table starts at +2
 * @param dest copy destination
 * @param streamIndex which stream to copy
 * @return the end of the copied data
 *
 * @romaddress 0x0800c794
 */
u8 *mission_script_copyStream(const u8 *scriptSection, u8 *dest, u8 streamIndex)
{
    const u8 *tbl = scriptSection + 2;
    const u8 *src;
    const u8 *op;
    u8 length;
    u8 i;

    tbl += streamIndex * 2;
    src = scriptSection + *(const u16 *)tbl;
    do
    {
        op = src;
        length = MissionScriptOpcodeLengths[*(const u32 *)op];
        for (i = 0; i < length; i++)
        {
            *dest++ = *src++;
        }
    } while (*(const u32 *)op != 0);
    return dest;
}
