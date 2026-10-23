#include "match_hacks.h"
#include "mission.h"
#include "variables.h"

/**
 * Copy a mission script stream, including its terminator, into the state area.
 *
 * @return The destination cursor past the copied records.
 *
 * @romaddress 0x0800c794
 */
u8 *mission_script_copyStream(const u8 *scriptSection, u8 *dest, u8 streamIndex)
{
    u32 offsetIndex = streamIndex * 2;
    const u8 *offsets = scriptSection + 2;
    u32 length = *(const u16 *)(offsets + offsetIndex);
    MATCH_PIN(const u8 *, cursor, "r2") = scriptSection + length;
    const u32 *record;
    const u8 *lengths = MissionScriptOpcodeLengths;
    u8 i;

    do
    {
        record = (const u32 *)cursor;
        length = *(const u8 *)(*record + (u32)lengths);
        for (i = 0; i < length; i++)
        {
            *dest++ = *cursor++;
        }
    } while (*record != 0);
    return dest;
}
