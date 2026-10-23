#include "font.h"
#include "patches/mod_ram/graphics_scratch.h"
#include "patches/mod_ram/memory_diagnostics.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

Heap g_Heaps[1];
u32 g_HeapCount = 1;
FontRenderCtx g_FontRenderCtx;
static _Alignas(HeapBlock) u8 arena[4096];
static _Alignas(HeapBlock) u8 tiles[0x800];

void *memory_memset(void *destination, u32 value, size_t size)
{
    return memset(destination, value, size);
}

void text_objRasterizeString(void *object, char **text)
{
    (void)object;
    (void)text;
}

static ModRamMemoryStats sample(void)
{
    ModRamMemoryStats stats;
    modRam_getMemoryStats(&stats);
    assert(stats.freeBytes <= stats.capacity);
    assert(stats.peakUsed >= stats.capacity - stats.freeBytes);
    return stats;
}

int main(void)
{
    HeapBlock *first = (HeapBlock *)arena;
    *first = (HeapBlock){0};
    g_Heaps[0] = (Heap){first, first, sizeof(arena), arena + sizeof(arena)};
    modRam_initMemoryStats();
    u32 capacity = sample().freeBytes;
    u8 *a = modRam_zalloc(65);
    assert(a && a[0] == 0 && a[64] == 0);
    memset(a, 0x5a, 65);
    void *b = modRam_malloc(128);
    assert(b && sample().allocations == 2);
    assert(modRam_realloc(a, 512) == NULL);
    modRam_free(b);
    assert(modRam_realloc(a, 512) == a && a[64] == 0x5a);
    assert(modRam_realloc(a, 32) == a && a[0] == 0x5a);
    modRam_free(a);
    assert(sample().freeBytes == capacity);
    a = modRam_malloc(capacity);
    assert(a && sample().freeBytes == 0);
    assert(modRam_realloc(a, capacity + 32) == NULL);
    assert(modRam_malloc(1) == NULL);
    modRam_free(a);
    assert(sample().freeBytes == capacity);
    assert(modRam_malloc(UINT32_MAX) == NULL);
    assert(sample().failures == 4 && sample().lastFailureSize == UINT32_MAX);
    modRam_resetMemoryStats();
    assert(sample().failures == 0 && sample().peakUsed == 0);

    // Exercise fragmentation, splitting and both coalescing directions.
    void *blocks[16] = {0};
    for (u32 iteration = 0; iteration < 512; iteration++)
    {
        u32 slot = (iteration * 7) % 16;
        if (blocks[slot])
        {
            modRam_free(blocks[slot]);
            blocks[slot] = NULL;
        }
        else
        {
            blocks[slot] = modRam_malloc(1 + iteration % 190);
            assert(blocks[slot]);
        }
        sample();
    }
    for (u32 i = 0; i < 16; i++)
    {
        modRam_free(blocks[i]);
    }
    assert(sample().freeBytes == capacity && sample().invalidFrees == 0);

    g_FontRenderCtx.tileScratch = tiles;
    void *map, *characters, *nested;
    const void *mapCaller = (void *)(((uintptr_t)text_objRasterizeString & ~(uintptr_t)1) + 0x1c);
    const void *tileCaller = (void *)(((uintptr_t)text_objRasterizeString & ~(uintptr_t)1) + 0x38);
    memset(tiles, 0xff, sizeof(tiles));
    assert(!modRam_borrowTextMap(NULL, &map));
    assert(modRam_borrowTextMap(mapCaller, &map));
    assert(!modRam_borrowTextMap(mapCaller, &nested));
    assert(!modRam_borrowTextTiles(tileCaller, 0xe80, &characters));
    assert(modRam_borrowTextTiles(tileCaller, 0x740, &characters));
    assert(characters == tiles && map == tiles + 0x780);
    assert(tiles[0] == 0 && tiles[0x73f] == 0 && tiles[0x740] == 0xff);
    assert(modRam_releaseTextScratch(map));
    assert(!modRam_borrowTextMap(mapCaller, &nested));
    assert(modRam_releaseTextScratch(characters));
    assert(modRam_borrowTextMap(mapCaller, &map));
    assert(modRam_releaseTextScratch(map));
    assert(modRam_sharedTextUses() == 2);
    puts("PASS: heap accounting, allocation failures, in-place resizing and shared text scratch");
    return 0;
}
