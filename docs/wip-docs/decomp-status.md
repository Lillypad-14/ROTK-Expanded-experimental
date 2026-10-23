# Latest frame-dependency pass

569 C definitions, 251 C regions, 72,292 compiled code bytes. All 453 manifest regions match vanilla, including whole-ROM SHA1. make check passed (297 tests, three default skips); configured Ghidra tests: 18 passed. Current evidence: frame-dependencies.md and frame-dependencies-check.log.

# Latest actor-render pass

567 C definitions,249 C regions,72,132 compiled code bytes. All451 manifest regions match vanilla, including whole-ROM SHA1. make check passed (297 tests, three default skips); configured Ghidra tests:18 passed. Current evidence: actor-render.md and actor-render-check.log.

# Latest verified pass

565 C definitions, 247 C regions, 71,912 compiled code bytes. All 449 manifest regions match vanilla and whole-ROM SHA-1 is unchanged. make check passed (297 tests, three default skips); configured Ghidra tests: 18 passed. Current evidence: mission-groups.md and mission-groups-final-check.log. The current portable handoff is docs/decomp-handoff.md.

Earlier pass notes below are historical.

# Latest verified mission-table pass

561 C definitions, 244 C regions, 71,454 compiled code bytes. All 446 manifest regions match vanilla and whole-ROM SHA-1 is unchanged. make check passed (297 tests, three default skips); configured Ghidra files passed all 18 tests. Logs: mission-tables-check.log and mission-tables-ghidra-tests.log. Current follow-up: mission-tables.md.

# Latest verified mission-loading pass

557 C definitions, 241 C regions, 70,940 compiled code bytes. All 443 manifest regions match vanilla, including whole-ROM SHA-1. make check passed (297 tests, three default skips); configured Ghidra test files: 18 passed. Logs: mission-load-check.log and mission-load-ghidra-tests.log. Current follow-up: mission-loading.md.

# Latest verified pass

555 C definitions, 239 C regions, 70,700 compiled code bytes. All 441 manifest regions and whole-ROM SHA-1 match vanilla. make check passed (297 tests, three default skips); configured Ghidra tests: 18 passed. Logs: stream-copy-check.log and stream-copy-ghidra-tests.log. See stream-copy-and-eeprom.md for current candidates.

# Latest resume: 2026-10-22

554 C definitions, 238 C regions, 70,632 compiled code bytes. Full ROM verified. See resume-2026-10-22.md and resume-check-final.log. Earlier sections below describe historical passes.

# Full decompilation status

The full decompilation goal remains active. These are verified incremental additions, not a completed game decompilation.

## Verified additions

- actor_tickAll, 0x080016f8, 112 bytes
- actor_parkMainList, 0x08001810, 40 bytes
- actor_setFlagsAll and actor_clearFlagsAll, 0x08001874, 76 bytes
- actor_allocOverlay, 0x08001918, 72 bytes
- pool_alloc, 0x08030518, 50 bytes
- pool_popFree and list_pushFront, 0x08030568, 48 bytes

The next core pass added pool_free, list_remove, actor_allocMain, system_endFrame, actor_restoreParkedMainList, and sprite_setOamMatrixIndex. These six functions add 322 bytes in four new regions and two extended regions. See core-decomp-followup.md for the detailed handoff.

The sprite lifecycle pass added eight more functions and 884 compiled bytes, including the completed sprite_free candidate, palette and mission-record cleanup, OAM matrix allocation, scale cleanup, and frame finalization. See sprite-core-decomp.md.

Twenty-two new functions and 1,604 bytes across these passes. All are in config/split.cfg and match the original ROM.

Current inventory: 359 C function definitions, 69 source regions, 55,770 compiled code bytes. The 822 mapped functions are an incomplete inventory, not a denominator for full-game progress. Extracted assets account for most of the ROM reconstruction coverage.

## Validation

make check passed: 297 tests passed, 3 Ghidra integration tests skipped by the default environment. Source/store consistency and BUGFIX compilation passed. Whole-ROM SHA-1 is ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd, identical to the base ROM.

The Ghidra test files also passed with Java and Ghidra configured: 18 passed, including the three integrations skipped by the default environment. See decomp-ghidra-tests.log.

The assembler generator now uses forward slashes in incbin filenames on Windows. Coverage now reports C code separately from asset regions.

## Tools

GhidrAssist is running read-only at http://127.0.0.1:8080/mcp. The installed client is ~/AppData/Local/rotk-tools/gh.py; gh_call.py in this directory accepts plain key=value arguments to avoid Windows quoting problems.

The installed Ghidra launcher is ~/AppData/Local/rotk-tools/start-ghidra-mcp.cmd. Do not launch a second instance against the same locked project.

The period compiler was rebuilt from ~/AppData/Local/rotk-tools/agbcc-src/gcc using Git Bash, make -j1 CC=clang SHELL=/bin/sh normal. Its binary is installed in tools/agbcc/bin/agbcc. ARM tools live in ~/AppData/Local/rotk-tools/arm-gcc/bin.

REA 6.7.0 was fetched and its CLI inspected through npm exec --yes --package=rea-agents -- rea --help. No client-wide REA configuration was needed because GhidrAssist already works.

## Next core work

Continue the actor lifecycle and rendering foundations: actor_tickAllInList, actor_tick, sprite_composeOam, and sprite_commitGfx. Follow with rotk_main and the startup dependencies it invokes. Ghidra output for rotk_main and actor_tickAllInList is saved here as decomp-*.txt. sprite_free is now verified source.

rotk_main contains several calls to functions not yet defined in the current Ghidra project. Recover those functions in Ghidra before assigning source-level names and signatures. Do not substitute generated recompilation output for verified decompiled C.

decomp_inventory.py regenerates decomp-inventory.json from the current sources and compiled regions. decomp-coverage.txt records the address-ordered region inventory. Preserve the pre-existing mod changes in this working tree.

## Claude handoff

The actor dispatcher, animation driver, and movement integration are now promoted and verified, adding 1,076 compiled bytes. Latest checks are actor-core-check.log and actor-core-ghidra-tests.log (18 passed). CLAUDE.md imports docs/decomp-handoff.md, which records the next targets and tooling. No processes remain pending from this pass.
