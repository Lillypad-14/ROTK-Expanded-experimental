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

Compared with the upstream base (commit `b4f8637`), counted from the repository files:

| | Upstream | This fork |
| --- | --- | --- |
| C source files in `src/c/` | 51 | 350 |
| Regions in `config/split.cfg` | 253 | 552 |
| Mapped functions in `config/functions.cfg` | 772 | 1051 |
| Data symbols in `config/data.cfg` | 356 | 483 |

The 299 new C files, by system:

| System | Files | Highlights |
| --- | --- | --- |
| Mission scripts | 85 | 63 script opcode handlers, plus the block, stream, waypoint, spawn-group and actor-record loaders |
| Actors | 39 | Allocation, movement, collision, death, stun, knockback, aggro, enemy spawners |
| Scenes | 38 | Shop, skill tree, new game, copyright and logo screens, victory |
| Sprites | 29 | OAM entries and affine matrices, palette and cell caches, animation binding |
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
