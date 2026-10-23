# actor_resolveTileCollision (0x080341a8) status

Semantics decoded; candidate local/wip-docs/tile/t2.c compiles, 477 insns differ (frame 0x3c vs ROM 0x4c, actor kept on stack instead of r8).
Asm dumps: tile-collision.asm.txt (main), tile-collision.tail.txt, tile-cases.bin (switch bodies; Ghidra will not disassemble them, decode with objdump -b binary -M force-thumb).
Config added: g_TileCollisionFlags/CenterType/ProbeTypes (0x0300205c..62), TileCollisionTypes (0x08283008, identity 0..7), actor_unknown08033e64, ACTOR_STATE_UNKNOWN_11.
Layout facts: ROM keeps saved coords at sp18, renderCoords copy at sp10, probes sp28/sp2c, edge count sp34, saved flags sp30.
Next: fix frame/register allocation; the ROM keeps actor in r8, probeX/Y in r6/r7, heading in r10.
sfx_play: best candidate sfx_play.best.c (5 insn diff: SfxEntries address calc order: ROM lsl idx then ldr base). Needs SfxEntries/SfxSingles/SfxVariantSets/SoundData rows in data.cfg (added), sfx_playUnlessPlaying p1 variant gets 9 diffs same cause. Config rows are in place already.
- gfx_fillBgRect/gfx_fillTilemapRect: candidates/gfx_fillBgRect.c, 15 insn diff each, regalloc only (ROM: wrapper uses 4 regs, base+0xc folded into ldr displacement; inner keeps width in r3 and tile in r4).
