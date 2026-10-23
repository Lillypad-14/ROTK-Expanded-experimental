#include "mission.h"
#include "variables.h"

typedef void (*MissionOpHandler)(u32 *record);

/**
 * Run a mission script stream until its terminator record.
 *
 * @romaddress 0x0800c4e0
 */
void mission_script_runStream(u8 streamIndex)
{
    u8 *streams;
    u8 *cursor;
    u8 *p;
    u32 i;
    const void **handlers;
    const u8 *lengths;
    u32 *record;
    u32 length;

    g_MissionCurrentStream = streamIndex;
    streams = g_MissionStreams;
    i = streamIndex * 2;
    p = streams + 2;
    cursor = streams + *(u16 *)(p + i);
    lengths = MissionScriptOpcodeLengths;
    handlers = MissionScriptOpcodeHandlers - 1;
    do
    {
        record = (u32 *)cursor;
        if (*record == 0)
        {
            length = lengths[0];
        }
        else
        {
            ((MissionOpHandler)handlers[*record])(record);
            length = *(u8 *)(*record + (u32)lengths);
        }
        cursor += length;
    } while (*record != 0);
}
