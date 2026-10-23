#pragma once

#include "lib/combat_event.h"

enum TestArenaCommand
{
    TEST_ARENA_RESET = 1,
    TEST_ARENA_DELIVERY = 2,
    TEST_ARENA_CAST = 3,
    TEST_ARENA_REFILL = 4,
    TEST_ARENA_CLEAR = 5,
    TEST_ARENA_WAIT = 6,
};

// Values of one automatic sweep row, in the order ARENA_ROW prints them.
enum TestArenaSweepValue
{
    TEST_ARENA_SWEEP_ROW,
    TEST_ARENA_SWEEP_ACCEPTED,
    TEST_ARENA_SWEEP_RECAST,
    TEST_ARENA_SWEEP_HITS,
    TEST_ARENA_SWEEP_KILLS,
    TEST_ARENA_SWEEP_DAMAGE,
    TEST_ARENA_SWEEP_PLAYER_HP,
    TEST_ARENA_SWEEP_LAG_TICKS,
    TEST_ARENA_SWEEP_WORST_TICK_VBLANKS,
    TEST_ARENA_SWEEP_HEAP_ROOM,
    TEST_ARENA_SWEEP_ALLOC_FAILURES,
    TEST_ARENA_SWEEP_INVALID_FREES,
    TEST_ARENA_SWEEP_BSS_GUARD_INTACT,
    TEST_ARENA_SWEEP_HERO_STATE_MISUSE,
    TEST_ARENA_SWEEP_EFFECTS_AFTER_CLEAR,
    // 1 when the player's HP reached 0 during the row, which reloads the mission.
    TEST_ARENA_SWEEP_PLAYER_DOWNED,
    TEST_ARENA_SWEEP_VALUE_COUNT,
};

// Boot settings in ROM. Tools patch them in a copy of the ROM image.
typedef struct TestArenaConfig
{
    // HeroId of the arena player.
    u32 hero;
    // Ticks each automatic sweep cast runs, or 0 to wait for debugger commands.
    u32 sweepTicks;
    // Encounter each automatic sweep cast starts from.
    u32 sweepFixture;
    // Ticks of the busy fight benchmark, or 0 for no benchmark.
    u32 benchTicks;
    // Living foes the benchmark keeps around the player.
    u32 benchCrowd;
    // 1 to sample the interrupted program counter about 1000 times a second while measuring.
    u32 benchSample;
} TestArenaConfig;

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
    // Game ticks that took more than two vblanks since the previous arena tick.
    u32 lagTicks;
    // Most vblanks between two consecutive arena ticks.
    u32 worstTickVblanks;
    u32 lastTickVblank;
    // Bytes between the highest heap allocation so far and the end of the heap.
    u32 heapRoom;
    u32 allocFailures;
    u32 invalidFrees;
    // 1 while the words below the mod .bss are untouched.
    u32 bssGuardIntact;
    u32 heroStateMisuse;
    // Tick at which a pending wait command completes.
    u32 waitUntil;
    // Active row of the automatic sweep.
    u32 sweepRow;
    // Steps of the active sweep row issued so far.
    u32 sweepStep;
    // Current sweep row, indexed by TestArenaSweepValue.
    u32 sweepValues[TEST_ARENA_SWEEP_VALUE_COUNT];
    // Tick of the latest reset command.
    u32 resetTick;
    // Benchmark phase: 0 before the crowd spawns, 1 warming up, 2 measuring, 3 finished.
    u32 benchPhase;
    // Tick at which the current benchmark phase began.
    u32 benchPhaseTick;
    // Vblank counter when measuring began.
    u32 benchStartVblank;
    // lagTicks when measuring began.
    u32 benchStartLag;
    // Sum of the living foe count over every measured tick.
    u32 benchFoeTicks;
    // Skills cast while measuring.
    u32 benchCasts;
    // Next active row the benchmark tries to cast.
    u32 benchRow;
    // Foes spawned so far, which also picks each spawn's position.
    u32 benchSpawned;
} TestArenaControl;

extern const volatile TestArenaConfig testArena_config;
extern volatile TestArenaControl *g_TestArena;
void testArena_boot(void);
void testArena_tick(u8 playerIndex);
