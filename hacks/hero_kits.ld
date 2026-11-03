/* The hero kits region (hacks/tools/hero_kit.py link): the spec kits, linked apart from the modcode
 * at MEM_HERO_KITS with its symbols (--just-symbols=hack.elf). The slot array comes first, so slot N
 * sits at MEM_HERO_KITS + N * sizeof(SkillExpansionKitSlot) whatever else changes. */

INCLUDE memmap.ld

SECTIONS
{
    . = MEM_HERO_KITS;
    .kits : { KEEP(*(.hero_kit_slots)) *(.rodata .rodata.* .data .data.*) }
    ASSERT(ADDR(.kits) == MEM_HERO_KITS, "the kit slots must start the region")
    ASSERT(. <= MEM_HERO_KITS_END + 1, "hero kits overflowed MEM_HERO_KITS (hacks/layout.cfg)")
    /* Data only: code belongs in the modcode, where the recompiler finds it. */
    .text : { *(.text .text.* .glue_7 .glue_7t) }
    ASSERT(SIZEOF(.text) == 0, "hero kits must hold no code")
    .bss (NOLOAD) : { *(.bss .bss.* COMMON) }
    ASSERT(SIZEOF(.bss) == 0, "hero kits must hold no mutable state")
    /DISCARD/ : { *(.ARM.exidx*) *(.comment) *(.note*) *(.ARM.attributes) *(.debug*) }
}
