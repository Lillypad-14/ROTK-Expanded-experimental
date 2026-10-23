#pragma once

#include "lib/combat_event.h"

enum TestArenaCommand
{
    TEST_ARENA_RESET = 1,
    TEST_ARENA_DELIVERY = 2,
    TEST_ARENA_CAST = 3,
    TEST_ARENA_REFILL = 4,
    TEST_ARENA_CLEAR = 5,
};

typedef struct TestArenaControl
{
    u32 magic;
    u32 version;
    u32 ready;
    u32 ticks;
    // Write arguments and command before publishing a new request number.
    u32 request;
    // Updated after the command has completed.
    u32 acknowledged;
    u32 command;
    // Reset: encounter 0..2. Delivery: recipe ID. Cast: active row.
    u32 argument;
    u32 level;
    u32 cosmetic;
    u32 result;
    u32 hits;
    u32 kills;
    u32 damage;
    s32 playerHp;
    s32 playerSpirit;
    Actor *targets[3];
    s32 targetHp[3];
} TestArenaControl;

extern volatile TestArenaControl *g_TestArena;
void testArena_boot(void);
void testArena_tick(u8 playerIndex);
