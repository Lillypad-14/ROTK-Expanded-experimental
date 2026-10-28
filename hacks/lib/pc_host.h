#pragma once

// Info block the ROTK Recompiled PC host writes at the start of its host RAM (0x12000000). Absent on
// a GBA, where the magic never matches, so every accessor falls back to the stock sizes.

#include "actor.h"
#include "types.h"

#define PC_HOST_INFO_ADDRESS 0x12000000
#define PC_HOST_INFO_MAGIC 0x31435052 // "RPC1"

typedef struct PcHostInfo
{
    // PC_HOST_INFO_MAGIC when the PC host is present
    u32 magic;
    // layout version, 3
    u32 version;
    // slots in the main actor pool (stock 0x100)
    u32 mainActors;
    // slots in the overlay actor pool (stock 0x28)
    u32 overlayActors;
    // main actor pool address
    u32 mainPool;
    // overlay actor pool address
    u32 overlayPool;
    // game heap start
    u32 heapBase;
    // game heap end
    u32 heapEnd;
    // enemies to spawn around the player for a stress test, counts down as they spawn (version 2)
    u32 stressSpawn;
    // the Versus duel's scoreboard, written by the mod every tick for the host to draw (version 3):
    // PC_HOST_DUEL_* fields, 0 outside a duel
    u32 duel;
} PcHostInfo;

#define PC_HOST_DUEL_PHASE(duel) ((duel) & 3)  // 1 = fighting, 2 = won
#define PC_HOST_DUEL_SCORE(duel, playerIndex) (((duel) >> (8 + 8 * (playerIndex))) & 0xff)
#define PC_HOST_DUEL_TARGET(duel) ((duel) >> 24)
#define PC_HOST_DUEL(phase, score0, score1, target)                                                    ((u32)(phase) | (u32)(score0) << 8 | (u32)(score1) << 16 | (u32)(target) << 24)

static inline const volatile PcHostInfo *hackLib_pcHostInfo(void)
{
#if defined(__arm__)
    const volatile PcHostInfo *info = (const volatile PcHostInfo *)PC_HOST_INFO_ADDRESS;
    return info->magic == PC_HOST_INFO_MAGIC ? info : NULL;
#else
    // Host unit tests have no guest address space.
    return NULL;
#endif
}

static inline volatile PcHostInfo *hackLib_pcHostInfoWritable(void)
{
    return (volatile PcHostInfo *)hackLib_pcHostInfo();
}

static inline u32 hackLib_mainActorCapacity(void)
{
    const volatile PcHostInfo *info = hackLib_pcHostInfo();
    return info != NULL ? info->mainActors : ACTOR_POOL_MAIN_CAPACITY;
}

static inline u32 hackLib_overlayActorCapacity(void)
{
    const volatile PcHostInfo *info = hackLib_pcHostInfo();
    return info != NULL ? info->overlayActors : ACTOR_POOL_OVERLAY_CAPACITY;
}
