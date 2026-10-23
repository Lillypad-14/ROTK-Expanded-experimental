# Stream copying and EEPROM follow-up

Promoted mission_script_copyStream, 0x0800c794, 68 bytes, to src/c and split manifest. It copies byte records using MissionScriptOpcodeLengths and includes the zero-opcode terminator. The prior n2 listing estimated 140 bytes from the next mapped symbol; the actual function ends at 0x0800c7d8 including its literal pool. Do not count unmapped intervening code as part of this function.

Allocation trace showed the loop counter winning r1 over record length. Reusing the initial stream offset local for opcode length and pinning cursor to r2 matches. All pin subsets were tested: cursor alone and both pins match, neither and length alone differ. A typed table subscript changes one adds operand order; the existing raw address expression matches and follows the adjacent runStream implementation.

EEPROM candidates remain unpromoted:
- start-pin-11.c: 24 differences, but a pinned r7 temporary is not preserved in the prologue; this variant is not acceptable source. start-reuse-0.c is a natural reuse candidate with 30 differences. start-trace contains compiler RTL and allocation traces. Initial candidate remains eeprom_startTimeoutTimer_candidate.c.
- DMA: natural eeprom_dmaBits_candidate.c has 28 differences. dma-pins2.c reduces them to 20, with too many controls to accept without further work. dma-trace contains the natural candidate allocation evidence. The original retains a copy of 0x8000 in r2 before the first volatile status read, then materializes a second copy for the loop after loading the status address into r2. CSE shares these in the natural source and derives the address from another hardware register base.
- eeprom_readBlock_candidate.c was reconstructed from Ghidra assembly and pseudocode, not yet matched. read-do-* and read-volatile.c are alternate probes. The original uses a 68-halfword stack bit buffer, emits read command/address bits backward, then decodes 64 response bits after four leading dummy bits into four halfwords in reverse destination order. Its trailing command buffer halfword is not initialized by this routine. Preserve original behavior before proposing any BUGFIX.

Added eeprom_dmaBits header prototype based on the assembly call convention. old_agbcc is not installed, so attempted --cc old_agbcc checks did not run. Do not interpret those failures as compiler comparison evidence.

Validation logs for this pass: stream-copy-check.log and stream-copy-ghidra-tests.log. Inventory generator updates decomp-inventory.json.
