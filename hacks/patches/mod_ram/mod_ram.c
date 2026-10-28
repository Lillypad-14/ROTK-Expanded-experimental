#include "memmap.h"
#include "memory.h"
#include "variables.h"
#include "patches/mod_ram/memory_diagnostics.h"

extern u8 __modcode_bss_start[];
extern u8 __modcode_bss_end[];

/**
 * Initialize the game heap, but a bit smaller in order to allow for a romhack .bss section.
 *
 * REPLACES (whole function): memory_init @0x08032ba0.
 */
void modRam_init(void)
{
#if PC_EDITION
    // The mod's globals are in host RAM, which system_clearRam leaves alone, and the heap keeps the
    // whole arena.
    memory_memset(__modcode_bss_start, 0, __modcode_bss_end - __modcode_bss_start);
    memory_addHeap(0, g_HeapArena, (u8 *)MEM_MODCODE_BSS_END + 1);
#else
    memory_addHeap(0, g_HeapArena, __modcode_bss_start);
#endif
    modRam_armBssGuard();
    modRam_initMemoryStats();
}
