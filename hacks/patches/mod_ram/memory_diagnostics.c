#include "memmap.h"
#include "patches/mod_ram/memory_diagnostics.h"
#include "patches/mod_ram/graphics_scratch.h"
#include "variables.h"

// Native heap helpers shared with the byte-matched implementation in src/c/memory.c.
extern u32 modRam_nativeUsableSize(HeapBlock *, u32) asm("usableSize");
extern void modRam_nativeLinkBlock(HeapBlock *, HeapBlock *) asm("linkBlock");
extern void modRam_nativeLinkFreeBlock(HeapBlock *, HeapBlock *) asm("linkFreeBlock");
extern void modRam_nativePushFreeBlock(HeapBlock *, u32) asm("pushFreeBlock");
extern void modRam_nativeUnlinkBlock(HeapBlock *, u32) asm("unlinkBlock");
extern void modRam_nativeUnlinkFreeBlock(HeapBlock *, u32) asm("unlinkFreeBlock");
extern void modRam_nativeMergeIntoPrevBlock(HeapBlock *, u32) asm("mergeIntoPrevBlock");
extern void modRam_nativeMergeNextBlock(HeapBlock *, u32) asm("mergeNextBlock");

#define HEAP_ALLOCATED ((HeapBlock *)-1)
#define HEAP_ALIGNMENT sizeof(HeapBlock)
#define ALIGN_REQUEST(size) (((size) + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1))

static ModRamMemoryStats stats;
extern u8 __modcode_bss_start[];

#define BSS_GUARD_VALUE 0x4d4f4447

u32 modRam_bssGuard[MOD_RAM_BSS_GUARD_WORDS] __attribute__((section(".bss.guard")));

void modRam_armBssGuard(void)
{
    for (u32 i = 0; i < MOD_RAM_BSS_GUARD_WORDS; i++)
    {
        modRam_bssGuard[i] = BSS_GUARD_VALUE ^ i;
    }
}

bool32 modRam_isBssGuardIntact(void)
{
    for (u32 i = 0; i < MOD_RAM_BSS_GUARD_WORDS; i++)
    {
        if (modRam_bssGuard[i] != (BSS_GUARD_VALUE ^ i))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void recordTop(const void *end)
{
    if ((u32)end > stats.peakTop)
    {
        stats.peakTop = (u32)end;
    }
}

static void sampleFreeSpace(void)
{
    u32 freeBytes = 0;
    u32 largest = 0;
    u32 i;

    for (i = 0; i < g_HeapCount; i++)
    {
        HeapBlock *block;
        for (block = g_Heaps[i].freeList; block != NULL; block = block->link.freeNext)
        {
            u32 size = modRam_nativeUsableSize(block, i);
            freeBytes += size;
            if (size > largest)
            {
                largest = size;
            }
        }
    }
    stats.freeBytes = freeBytes;
    stats.largestBlock = largest;
}

static void recordUsed(void)
{
    u32 used = stats.capacity - stats.freeBytes;
    if (used > stats.peakUsed)
    {
        stats.peakUsed = used;
    }
}

void modRam_initMemoryStats(void)
{
    memory_memset(&stats, 0, sizeof(stats));
    sampleFreeSpace();
    stats.capacity = stats.freeBytes;
#if PC_EDITION
    stats.heapEnd = MEM_MODCODE_BSS_END + 1;
#else
    stats.heapEnd = (u32)__modcode_bss_start;
#endif
}

void modRam_getMemoryStats(ModRamMemoryStats *out)
{
    sampleFreeSpace();
    recordUsed();
    *out = stats;
}

void modRam_resetMemoryStats(void)
{
    u32 capacity = stats.capacity;
    u32 peakTop = stats.peakTop;
    modRam_initMemoryStats();
    stats.capacity = capacity;
    stats.peakTop = peakTop;
    recordUsed();
}

static void failed(size_t size, const void *caller)
{
    stats.failures++;
    stats.lastFailureSize = size;
    stats.lastFailureCaller = caller;
}

static void *allocate(size_t request, const void *caller)
{
    u32 i;
    u32 size;

    if (request > (u32)-1 - HEAP_ALIGNMENT + 1)
    {
        failed(request, caller);
        return NULL;
    }
    size = ALIGN_REQUEST(request);
    for (i = 0; i < g_HeapCount; i++)
    {
        HeapBlock *block;
        for (block = g_Heaps[i].freeList; block != NULL; block = block->link.freeNext)
        {
            u32 usable = modRam_nativeUsableSize(block, i);
            if (usable >= size)
            {
                u32 cost = size;
                if (usable > size)
                {
                    HeapBlock *tail = (HeapBlock *)((u8 *)(block + 1) + size);
                    modRam_nativeLinkBlock(block, tail);
                    modRam_nativeLinkFreeBlock(block, tail);
                    cost += sizeof(HeapBlock);
                }
                modRam_nativeUnlinkFreeBlock(block, i);
                block->link.heapIndex = i;
                block->freePrev = HEAP_ALLOCATED;
                stats.allocations++;
                stats.freeBytes -= cost;
                recordUsed();
                recordTop((u8 *)(block + 1) + size);
                return block + 1;
            }
        }
    }
    failed(request, caller);
    return NULL;
}

void *modRam_malloc(size_t size)
{
    void *scratch;
    if (modRam_borrowTextMap(__builtin_return_address(0), &scratch))
    {
        return scratch;
    }
    return allocate(size, __builtin_return_address(0));
}

void *modRam_zalloc(size_t size)
{
    void *scratch;
    if (modRam_borrowTextTiles(__builtin_return_address(0), size, &scratch))
    {
        return scratch;
    }
    void *pointer = allocate(size, __builtin_return_address(0));
    if (pointer != NULL)
    {
        memory_memset(pointer, 0, ALIGN_REQUEST(size));
    }
    return pointer;
}

void modRam_free(void *pointer)
{
    HeapBlock *block;
    u32 index;
    u32 reclaimed;

    if (pointer == NULL || modRam_releaseTextScratch(pointer))
    {
        return;
    }
    block = (HeapBlock *)pointer - 1;
    index = block->link.heapIndex;
    if (index >= g_HeapCount || block->freePrev != HEAP_ALLOCATED)
    {
        stats.invalidFrees++;
        return;
    }
    reclaimed = modRam_nativeUsableSize(block, index);
    if (block->next != NULL && block->next->freePrev != HEAP_ALLOCATED)
    {
        reclaimed += sizeof(HeapBlock);
    }
    if (block->prev != NULL && block->prev->freePrev != HEAP_ALLOCATED)
    {
        reclaimed += sizeof(HeapBlock);
    }
    modRam_nativePushFreeBlock(block, index);
    modRam_nativeMergeNextBlock(block, index);
    modRam_nativeMergeIntoPrevBlock(block, index);
    stats.freeBytes += reclaimed;
}

void *modRam_realloc(void *pointer, size_t request)
{
    HeapBlock *block;
    u32 index;
    u32 usable;
    u32 size;

    if (pointer == NULL)
    {
        return allocate(request, __builtin_return_address(0));
    }
    block = (HeapBlock *)pointer - 1;
    index = block->link.heapIndex;
    if (index >= g_HeapCount || block->freePrev != HEAP_ALLOCATED ||
        request > (u32)-1 - HEAP_ALIGNMENT + 1)
    {
        failed(request, __builtin_return_address(0));
        return NULL;
    }
    size = ALIGN_REQUEST(request);
    usable = modRam_nativeUsableSize(block, index);
    if (size < usable)
    {
        HeapBlock *tail = (HeapBlock *)((u8 *)pointer + size);
        u32 reclaimed = usable - size - sizeof(HeapBlock);
        if (block->next != NULL && block->next->freePrev != HEAP_ALLOCATED)
        {
            reclaimed += sizeof(HeapBlock);
        }
        modRam_nativeLinkBlock(block, tail);
        modRam_nativePushFreeBlock(tail, index);
        modRam_nativeMergeNextBlock(tail, index);
        stats.freeBytes += reclaimed;
    }
    else if (size > usable)
    {
        HeapBlock *next = block->next;
        u32 combined;

        // The native implementation dereferences next when the allocation is the last block.
        if (next == NULL || next->freePrev == HEAP_ALLOCATED ||
            size > (combined = usable + modRam_nativeUsableSize(next, index) + sizeof(HeapBlock)))
        {
            failed(request, __builtin_return_address(0));
            return NULL;
        }
        if (size < combined)
        {
            HeapBlock *tail = (HeapBlock *)((u8 *)pointer + size);
            modRam_nativeLinkBlock(next, tail);
            modRam_nativeLinkFreeBlock(next, tail);
        }
        modRam_nativeUnlinkFreeBlock(next, index);
        modRam_nativeUnlinkBlock(next, index);
        stats.freeBytes -= size - usable - (size == combined ? sizeof(HeapBlock) : 0);
        recordUsed();
        recordTop((u8 *)pointer + size);
    }
    return pointer;
}
