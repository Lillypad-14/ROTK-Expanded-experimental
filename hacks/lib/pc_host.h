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
    // layout version, 2
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
} PcHostInfo;

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
