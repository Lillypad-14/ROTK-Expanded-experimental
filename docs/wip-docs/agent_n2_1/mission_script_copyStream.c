#include "mission.h"
#include "variables.h"

/**
 * Copy one stream's records into the stream state.
 *
 * @return state-area cursor past the stream
 *
 * @romaddress 0x0800c794
 */
u8 *mission_script_copyStream(const u8 *scriptSection, u8 *dest, u8 streamIndex)
{
    u32 idx = streamIndex * 2;
    const u8 *table = scriptSection + 2;
    const u8 *src = scriptSection + *(u16 *)(table + idx);
    const u32 *record; u8 i; u8 len; do { record = (const u32 *)src; len = MissionScriptOpcodeLengths[*record]; for (i = 0; i < len; i++) { *dest++ = *src++; } } while (*record != 0); return dest;
}
