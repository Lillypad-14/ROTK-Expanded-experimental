#include "mission.h"

/**
 * Copy serialized actor parameters after a runtime mission-record header.
 *
 * @romaddress 0x0800c728
 */
void mission_script_copyActorRecord(const u8 *source, u8 *dest, u8 size)
{
    u8 i;

    dest += sizeof(MissionActorRecordHeader);
    for (i = 0; i < size; i++)
    {
        *dest++ = *source++;
    }
}
