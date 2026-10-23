#include "gba.h"
#include "gfx.h"
#include "variables.h"

extern const u8 decompress_lz77[];
extern const u8 decompress_pucrunch[];

/**
 * Copy the ARM LZ77 and pucrunch decoders from ROM into their IWRAM buffers and record the
 * IWRAM entry points.
 *
 * @romaddress 0x08019d04
 */
void decompress_loadIwramFunctions(void)
{
    bios_cpuSet(decompress_lz77, g_DecompressLz77Code, CPU_SET_32BIT | 0x7e);
    g_DecompressLz77 = (u32)g_DecompressLz77Code;
    bios_cpuSet(decompress_pucrunch, g_DecompressPucrunchCode, CPU_SET_32BIT | 0xd4);
    g_DecompressPucrunch = (u32)g_DecompressPucrunchCode;
}
