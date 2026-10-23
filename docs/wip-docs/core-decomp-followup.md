# Core decompilation follow-up

The preceding goal turn made verified progress: eight functions matched and entered the reconstruction manifest. This turn extends that work with six more functions, 322 additional bytes.

## Added verified source

- pool_free, 0x0803054c, appended to src/c/pool_alloc.c. The region is now 80 bytes.
- list_remove, 0x08030598, appended to src/c/pool_list.c. The region is now 92 bytes.
- actor_allocMain, 0x0800169c, 92 bytes.
- system_endFrame, 0x08001768, 76 bytes.
- actor_restoreParkedMainList, 0x08001838, 60 bytes.
- sprite_setOamMatrixIndex, 0x080365c0, 20 bytes.

All six region matches were rerun after clang-format and passed. The new functions are in config/split.cfg. Expected inventory after building: 348 C definitions, 61 C regions, 53,810 bytes. Current function map has 814 entries.

## Analysis tooling discovery

GhidrAssist disassemble_bytes works on undecoded functions even where create_function and get_pseudocode return failure. It prints Thumb instructions using the current program context. Use gh_call.py disassemble_bytes address=0x... to inspect missing callees. No terminal disassembler was needed.

gh_schema.py reads complete MCP schemas through the existing client. It is useful because gh.py --list truncates schemas.

## Sprite destruction candidate

local/wip-docs/sprite_free_candidate.c reconstructs sprite_free, 0x080009e4. It is deliberately outside src/c and the manifest because it is not yet byte-matched. The current match has 19 differing instructions, all register choices, with the same instruction sequence, addresses, and region length as the ROM. core-sprite_free.diff.txt holds the full comparison before formatting; core-sprite_free-current.txt holds the compact current comparison.

Remaining differences:

- Initial sentinel constant lives in r5 instead of r6, and the flags pointer in r6 instead of r5.
- The main tile-number address is initially r3 instead of r2, and spriteFlags lives in r2 instead of r3. This propagates into the frame-selection block and the dynamic-tile free call.

The remaining work is allocation topology, not behavior or branch layout. Compiler artifacts and allocation traces are in local/wip-docs/core-match/sprite_free. Regenerate them for the current candidate with --keep before relying on pseudo IDs.

Source forms that moved toward the original:

- Keep the unused-actor return at the end, with the body inside the non-unused branch.
- Extract affineMode into a separate u8 local before comparing 1 and 3. Direct repeated bitfield access emits an AND instead of the original shift pair.
- Cache the sprite cell's byte offset, not its array index. The inline cacheEntry helper preserves the original offset-plus-base operand order and reloads the global cache after the free call.
- Load the cache base before multiplying the cell offset by sizeof(SpriteCellCacheEntry).
- Capture prefix --cacheEntry(cell)->refCounts[frame] into a u32. This preserves the store followed by the original shift-only zero test. A separate u8 decrement adds narrowing instructions; reloading the stored byte adds a load.
- Sentinel writes in this routine use |= 0xffff, as shown by the original OR instructions.

## Discovered dependencies

- mission_script_setRecordActor, 0x0800c648: writes a passed Actor pointer to a group/record selected through two relative-offset tables. The destructor passes NULL for mission-spawned actors. Named and declared, not decompiled yet.
- sprite_releasePalette, 0x080382a8: checks ACTOR_FLAG_DEFAULTS_APPLIED, decrements a palette-indexed reference count, and schedules a release countdown for palette indices above 8 when the count reaches zero. Named and declared, not decompiled yet.
- SpriteCellCacheEntry has a 44-byte stride. Observed fields are owner pointer, gfxSize, tileNumbers starting at byte 6, and frame reference counts starting at byte 30. The 660-byte cache allocation and clear are at 0x080006aa and actor_restoreParkedMainList. The typed cache supports ongoing sprite_free/actor_tick/sprite_commitGfx work; examine remaining cache consumers to confirm all bounds.
- g_SpritePostCommitQueue is the 256-entry actor pointer queue at 0x030015e8; its u32 count is at 0x030019e8. system_endFrame clears ANIMATION_FRAME_CHANGED on queued actors before deferred tile frees.

## Validation results

make check passed: 297 tests passed, three Ghidra integrations skipped in the default environment, source/store checks passed, BUGFIX compilation passed, and the whole-ROM SHA-1 equals ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd. The configured Ghidra test files also passed all 18 tests, including those three integrations. Logs are core-check.log and core-ghidra-tests.log. Both exec processes are terminal.

The refreshed inventory confirms 348 C definitions, 61 C regions, 53,810 compiled code bytes, and 814 mapped functions. The full decompilation goal remains active.
