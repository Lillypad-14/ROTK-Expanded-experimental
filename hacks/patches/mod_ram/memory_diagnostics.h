#pragma once

#include "memory.h"

typedef struct ModRamMemoryStats
{
    u32 capacity;
    u32 freeBytes;
    u32 largestBlock;
    u32 peakUsed;
    u32 failures;
    u32 lastFailureSize;
    const void *lastFailureCaller;
    u32 allocations;
    u32 invalidFrees;
} ModRamMemoryStats;

void modRam_initMemoryStats(void);
void modRam_getMemoryStats(ModRamMemoryStats *out);
void modRam_resetMemoryStats(void);
void *modRam_malloc(size_t size);
void *modRam_zalloc(size_t size);
void *modRam_realloc(void *pointer, size_t size);
void modRam_free(void *pointer);
