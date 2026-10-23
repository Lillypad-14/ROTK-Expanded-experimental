#pragma once

#include "types.h"

typedef struct ActorMissionTimerState
{
    u32 field_0x0;
    u16 field_0x4;
    u16 field_0x6;
    u16 field_0x8;
    u8 field_0xa;
    u8 field_0xb;

    /** spawn group started when the countdown expires */
    u8 groupIndex;

    /** stream started when the countdown expires */
    u8 streamIndex;

    u8 field_0xe;
    u8 field_0xf;
} ActorMissionTimerState;
