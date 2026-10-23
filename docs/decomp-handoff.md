# Decompilation handoff

## Objective and verified state

Continue fully decompiling Return of the King (GBA) into readable C, verifying reconstructed regions against the original ROM.
Prioritize core systems, then gameplay dependencies.
The user authorized implementation and tooling work and resumed Codex after Claude's progress.
The full decompilation remains incomplete.

Verified on 2026-10-23: 710 C function definitions, 355 C source regions, and 88,760 compiled code bytes.
There are 1,058 mapped function symbols, an incomplete inventory that cannot serve as a full-game denominator.
Asset reconstruction coverage is not C decompilation progress.

The base ROM is `rotk.gba` in this repository.
It matches the user's fresh `../rotk_VANILLAROM.gba` exactly.
The reconstructed `build/rom.gba` also has SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`.
Hack changes use a separate build path.

Latest validation:

- `make check`: passed, including all 557 manifest regions, source/store consistency, BUGFIX compilation and 297 tests with three default-environment Ghidra skips.
- Configured Ghidra header/symbol test files: 18 passed, including the three integrations skipped by default.
- Whole-ROM SHA-1: identical to the base ROM.

The latest additions are `sprite_buildOamMatrices` (68 bytes) and `fx_renderLayer` (92 bytes), both plain C.
The particle queue builder and particle tick/render routine are mapped but still need C implementations.

Other recent actor additions are `actor_updateRenderFollow` (108 bytes) and `actor_applyRenderOscillation` (112 bytes).
Both are verified in the full ROM, with typed scratch views preserving the actor layout.

Recent mission work includes the block loader, spawn-record copier, stream copier and loader, waypoint-path copier and loader, spawn-group copier and loader, actor-record copier and two alignment helpers.
The mission group loader and copier share `src/c/mission_script_groups.c`.
All are in the manifest and match the original bytes.
The group functions need no matching macros.
The waypoint copier needs two register pins and a fresh-value barrier for its final size calculation; every smaller subset was tested and differed.

## Repository rules

Read `AGENTS.md`, `docs/conventions.md`, `docs/workflow.md`, and `.agents/skills/byte-matching/SKILL.md`.
Start reverse engineering through GhidrAssist MCP.
Do not rename or annotate Ghidra symbols; author persistent names in config and headers.
Handwritten source lives in `src/c/`, `include/`, and `config/`.
Do not hand-edit carved files.
Document decompiled functions on their C definitions, including `@romaddress`, and remove their prototype documentation.
Use the normal agbcc flags and require a leading `MATCH` verdict before adding a region to the manifest.
Keep speculative research in gitignored `local/wip-docs/`.
Do not use em dashes in repository files.
Do not change Git state without explicit instructions.

This working tree contains substantial pre-existing mod work and untracked files, as well as the decompilation additions.
Preserve it all; do not clean, reset, stash, or overwrite unrelated work.
No commits were created for this handoff.

## Local tooling

Workspace: `<repo>`.
Use PowerShell, with the ARM tools on PATH:

```powershell
$env:PATH='<tools-dir>/arm-gcc/bin;<tools-dir>/armips;' + $env:PATH
uv run rotkit match actor_tick src/c/actor_tick.c --full
& 'C:/Program Files/Git/bin/bash.exe' -lc 'make check'
```

The installed compiler is `tools/agbcc/bin/agbcc`.
The previously recorded external compiler-source directory is no longer present.
The installed compiler still provides matching and allocation traces through `rotkit match --keep`.
`tools/rotkit/build/rom.py` includes a Windows forward-slash incbin fix; preserve it.
`tools/rotkit/coverage.py` reports C bytes separately from asset regions.

GhidrAssist was confirmed available at `http://127.0.0.1:8080/mcp` during this handoff.
Its local client is `<tools-dir>/gh.py`.
The optional scratch wrapper accepts calls such as:

```powershell
python local/wip-docs/gh_call.py get_pseudocode function=actor_tick
python local/wip-docs/gh_call.py disassemble_bytes address=0x08000c38 length=288
```

Use `disassemble_bytes` for functions not yet defined in the Ghidra project.
Do not launch a second Ghidra against a locked project.
The launcher is `<tools-dir>/start-ghidra-mcp.cmd`.
REA (`https://github.com/morluto/rea`) was inspected as requested; the working GhidrAssist connection already supplies the reverse-engineering workflow.

For Ghidra integration tests:

```powershell
$env:JAVA_HOME='<tools-dir>/jdk/jdk-21.0.12.1+1'
$env:GHIDRA_INSTALL_DIR='<tools-dir>/ghidra/ghidra_12.1.4_PUBLIC'
$env:PATH=$env:JAVA_HOME+'/bin;' + $env:PATH
uv run --group dev pytest tools/rotkit/ghidra_build/test_parse_headers.py tools/rotkit/ghidra_build/test_apply_symbols.py -q
```

## Next work

`actor_tickAllInList`, `actor_getFrameGfx`, `sprite_getFrameCompression`, `sprite_uploadFrame` and `sprite_commitGfx` now match. Continue with `sprite_composeFrame` (0x08002cd8, best candidate is 92 instructions off) and `sprite_composeOam`.
Inspect the current source/manifest before selecting a target, since progress may continue outside this chat.
Then recover `rotk_main` and its startup dependencies through Ghidra.
`decompress_chunk` has a verified signature in `include/decompress.h`, but no C body yet.

The EEPROM watchdog-start, DMA and block-read routines still have local candidates that differ from the ROM.
Do not count or promote them until they match.
The standard compiler is installed; `old_agbcc` is not installed, so previous attempts to compare it did not run.

## Optional local evidence

These scratch files are not build dependencies:

- `local/wip-docs/decomp_inventory.py` and `decomp-inventory.json`: source and compiled-region inventory.
- `local/wip-docs/frame-dependencies-check.log`: latest complete project checks.
- `local/wip-docs/frame-dependencies-ghidra-tests.log`: latest configured Ghidra tests.
- `local/wip-docs/frame-dependencies.md`: current particle and actor-frame dependencies.
- `local/wip-docs/actor-render.md`: render-follow and oscillation reconstruction evidence.
- `local/wip-docs/mission-groups.md`: mission reconstruction evidence and matching-control audit.
- `local/wip-docs/mission-tables.md`, `mission-loading.md` and `stream-copy-and-eeprom.md`: earlier research, superseded by current `src/c/` files where functions have been promoted.

Scratch candidates and probes are not authoritative source.
Recheck current files and tool results rather than treating these notes as proof of future state.
