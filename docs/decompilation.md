# Original-game decompilation

The goal is readable C that reproduces the original GBA program byte for byte. The project is incomplete. Unimplemented spans remain included from a developer's own base ROM, so a successful whole-ROM rebuild does not mean the entire game has been decompiled.

The [generated source inventory](source-inventory.md) lists every current C translation unit, its manifest address, its function names, mapped functions without C, and reconstructed data regions. It also separates the expansion patches and tooling from original-game code.

## Where to look

| Area | Source of truth |
| --- | --- |
| Original-game C implementations and function descriptions | [`src/c/`](../src/c/) |
| Recovered structures and function signatures | [`include/`](../include/) |
| Original function addresses, modes and names | [`config/functions.cfg`](../config/functions.cfg) |
| Handwritten data symbols | [`config/data.cfg`](../config/data.cfg) |
| Regions compiled into the reconstructed ROM | [`config/split.cfg`](../config/split.cfg) |
| Carved data tables and generated symbol stores | [`carved/`](../carved/) |
| Parsing, carving, matching and reconstruction tools | [`tools/rotkit/`](../tools/rotkit/) |

The source covers parts of actors and sprites, combat and player stats, active/passive skills, mission scripting, menus and scenes, text and graphics, effects, inventory and loot, saves, sound and system services. Each subsystem remains subject to gaps. Use the inventory's function lists rather than interpreting a subsystem name as a completion claim.

## What this fork added

Compared with the upstream base (commit `b4f8637`), counted from the repository files on 2026-10-25:

| | Upstream | This fork |
| --- | --- | --- |
| C source files in `src/c/` | 51 | 356 |
| Regions in `config/split.cfg` | 253 | 558 |
| Mapped functions in `config/functions.cfg` | 772 | 1059 |
| Handwritten data symbols in `config/data.cfg` | 356 | 487 |

The 305 new C files, by system:

| System | Files | Highlights |
| --- | --- | --- |
| Mission scripts | 85 | 63 script opcode handlers, plus the block, stream, waypoint, spawn-group and actor-record loaders |
| Actors | 41 | Allocation, movement, collision, death, stun, knockback, aggro, enemy spawners |
| Scenes | 38 | Shop, skill tree, new game, copyright and logo screens, victory |
| Sprites | 33 | OAM entries and affine matrices, palette and cell caches, animation binding |
| Graphics | 27 | BG layers, scrolling, rotation and zoom, color effects, palette fades and cycles |
| Skills | 17 | Active skill ticks and spirit costs |
| Player | 16 | HUD, stats and scripted player actors |
| RNG | 10 | The gameplay RNG and its MT19937 source |
| Effects | 8 | Particle rendering and status effects |
| Multiplayer, EEPROM, text, sound, save and others | 30 | |

New headers: `actor/missionTimer.h`, `actor/projectile.h`, `decompress.h` and `terrain.h`. Many existing headers
gained recovered structs and enums, for example the mission file format (`MissionBlockHeader`, `MissionSpawnRecord`,
`MissionWaypointPath`), the BG layer state (`BgLayer`, `BgState`), FX particles (`FxParticle`, `FxDesc`) and
sprite animation commands.

Tooling changes:

- `tools/rotkit/build/rom.py` writes `incbin` paths with forward slashes, so the build works on native Windows.
- `tools/rotkit/coverage.py` reports C bytes separately from asset regions.
- The `-DBUGFIX` check in `make/decomp.mk` uses temporary files safely.
- The [byte-matching skill](../.agents/skills/byte-matching/SKILL.md) gained the agbcc quirks found while
  matching these functions.

## Measuring progress

On 2026-10-25, coverage reported **711 C function definitions in 356 source regions, 89,644 compiled bytes, and 1,059 mapped functions**. The regenerated inventory lists 348 mapped functions without C and 202 non-C reconstruction regions. The latest original-ROM verification matched all 558 manifest regions and the whole-ROM SHA-1.

On 2026-10-23, `make coverage` reported 704 C function definitions in 350 source regions, 86,452 compiled bytes and 1,051 mapped functions. The [2026-10-22 handoff](decomp-handoff.md) reported 569 C definitions, 251 C regions and 72,292 compiled code bytes. Those are dated results, not live counters. Regenerate the inventory for current source counts.

```sh
uv run python tools/documentation_inventory.py
make coverage
```

The first command needs the Python project environment but neither a ROM nor the matching compiler. The second uses compiled reconstruction regions and reports byte sizes and base-ROM gaps.

Do not divide C counts by mapped function counts to claim completion: the function map is incomplete. Do not report reconstructed assets as decompiled code. Mod code is new behavior, not recovered original-game C.

## Verifying a contribution

Follow [setup](setup.md), [workflow](workflow.md) and [conventions](conventions.md). Start native reverse engineering in Ghidra, persist discovered symbols in config and headers, then author and byte-match the implementation:

```sh
uv run rotkit match FunctionName src/c/module.c --full
make verify
make check
```

Add the source region to `config/split.cfg` once matching succeeds. A definition without manifest membership is not included in reconstruction checks. Inspect each command's result rather than treating the presence of an old build artifact as verification.

`make verify` produces `build/rom.gba`, which must match SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`. Hack outputs use a separate path and intentionally differ from that ROM. Matching requires the repository's agbcc toolchain; the expansion build uses ARM GCC. See setup for both.

## Actor and sprite performance groundwork

`actor_tickAllInList` at `0x08000708` is implemented in `src/c/actor_tickAllInList.c` and matches all 732 ROM bytes without matching hacks.
Its caller `actor_tickAll` and the existing 660-byte `actor_tick` implementation also retain exact matches.
The dispatcher includes forward and reverse actor traversal, collision dispatch, render-follow updates, fixed and sorted sprite submission, particle layers, mirrored sprites and hit flashes.

The native startup copy code places the render sorter at `0x03002c30` (248 bytes) and collision code at `0x03002d30` (1244 bytes).
Both already execute from IWRAM.
The OBJ tile-search routine at `0x0800e17c` is also copied to IWRAM at `0x03007690` (140 bytes).
These buffers are named in `config/data.cfg`, rather than treated as free memory.
The dispatcher has a word-sized return type, matching the original generated code.

The remaining sprite target is `sprite_composeOam` at `0x08002484`.
Its `sprite_composeFrame` helper at `0x08002cd8` is matched (884 bytes), including affine scaling, flips, clipping, priority adjustment and OAM submission.
These byte-matched ports alone add no new IWRAM placement.
The separate [OBJ tile-search optimization](performance.md) changes an existing ARM/IWRAM routine and has measured stress-test gains.
Placement still requires a verified memory range and equivalent behavior across gameplay, overlays and co-op.

Four sprite dependencies are also matched: `actor_getFrameGfx` (36 bytes), `sprite_getFrameCompression` (50 bytes), and the two upload routines in `sprite_uploadFrame.c` (224 bytes combined).
They cover packed frame tile offsets, RL/LZ77 selection, raw tile copying, BIOS RL decompression and the native IWRAM LZ77 decoder.
The upload paths retain the original tile addresses and transfer sizes.
`sprite_commitGfx` at `0x0800304c` is also byte-matched and included in reconstruction.
It preserves shared-cell and per-frame tile caches, deferred tile releases, actor frame uploads and attached sprite frames.
Its function body is 1,266 bytes, with alignment padding preserved by the reconstruction build.
