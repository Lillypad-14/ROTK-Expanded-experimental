#include "memmap.h"
#include "memory.h"
#include "variables.h"
#include "patches/mod_ram/memory_diagnostics.h"

extern u8 __modcode_bss_start[];

/**
 * Initialize the game heap, but a bit smaller in order to allow for a romhack .bss section.
 *
 * REPLACES (whole function): memory_init @0x08032ba0.
 */
void modRam_init(void)
{
    memory_addHeap(0, g_HeapArena, __modcode_bss_start);
    modRam_armBssGuard();
    modRam_initMemoryStats();
}
