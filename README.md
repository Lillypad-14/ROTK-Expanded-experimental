# ROTK Expanded (experimental)

An experimental content mod for _The Lord of the Rings: The Return of the King_ (GBA), built on top of the
[lotr-gba-re/rotk](https://github.com/lotr-gba-re/rotk) decompilation and ROM hack framework.

> [!important]
> **Credit for the foundation goes to [RagingCactus](https://github.com/RagingCactus)** and the
> [lotr-gba-re/rotk](https://github.com/lotr-gba-re/rotk) project and discord community: the reverse engineering, the byte-matched
> decompilation, the `rotkit` toolkit, the modular hack build, and the original patches (extra save slots, mod
> options, loot QoL, chapter select, fast fades, and more).
> This repository adds experimental content on top of that work. Please support and contribute to the original project
> and join the [LOTR GBA Discord community](https://discord.gg/yjHm8uf49e).


## What this mod adds

- **Second skill page for every hero**, including Eowyn's experimental Against the Dark kit. Expanded active and passive skill trees have three color-coded
  builds per page and a unique mechanic per hero:
  - Aragorn, Frodo (Burden meter on the HUD), Sam ("Samwise's Garden"), Smeagol ("Gollum's Grip"),
    Gimli ("The Tally"), Gandalf ("The Grey and the White", hold R to swap form), Legolas ("The Glint", timed
    Overdraw release for True Shots).
- **New playable heroes.** Ringwraith (hero 8, built from the Nazgul model, with its own gear) and Saruman
  (empowered spells).
- **Gauntlet dungeon mode.** Existing maps, randomized doors and encounters, a gear buy/sell shop, 94 boons, character-specific rewards, and evolving boon goals. The current pool has 28 available maps. See [Gauntlet](docs/gauntlet.md) and [the boon catalog](docs/gauntlet-boons.md).
- **Twists.** Playthrough modifier pages in the mod options. See [mod options](docs/mod-options.md).
- **Debug cheats and Unlocks pages** for testing.
- **Content toolkit** in `hacks/lib/`: FX kit (stock effects, recolors, sprite tints, custom PNG effects), combat
  events, targeting, movement, status effects, timed zones, summons, HUD meters and cast timelines.
- Engine and save changes needed for all of the above (skill system, skill tree screen, combat and per-frame hooks).

The v0.6 audit passed 280 automated combat-room clears across all ten heroes and the 28 available maps, plus ability, shop, memory and host regression checks. Manual balance, complete visual, campaign and hardware testing remain open. See [the audit and its limits](docs/bug-audit.md).

## Heroes

Quick version: [HEROES.txt](HEROES.txt).

Each hero below has a second skill page with three builds and a mechanic of its own. Eowyn's new page uses Odds tiers and Gallop and still needs manual balance testing. Full rundowns with every skill are in
**[docs/heroes.md](docs/heroes.md)**.

| Hero | Mechanic | In short |
| --- | --- | --- |
| [Aragorn](docs/heroes.md#aragorn) | Flame of Anduril | Hits, stuns and marks fill a Flame bar that powers the King's abilities. |
| [Frodo](docs/heroes.md#frodo) | Burden | The Ring's corruption is a HUD meter: power that rises in combat and carries between zones. |
| [Sam](docs/heroes.md#sam) | Samwise's Garden | Plant taters and brambles that grow and ripen. Ripe taters pelt foes, harvests restore Spirit. |
| [Smeagol](docs/heroes.md#smeagol) | Gollum's Grip | Fill Grip to 100 and Gollum takes over for two seconds at double damage. |
| [Gimli](docs/heroes.md#gimli) | The Tally | Page 2 is bought with kills. Every 10 kills charges a free, stronger cast. |
| [Gandalf](docs/heroes.md#gandalf) | The Grey and the White | Hold R to swap form. Grey saves Spirit, White burns it for +30% power. |
| [Legolas](docs/heroes.md#legolas) | The Glint | Release Overdraw on the glint for a True Shot and chain a five-shot streak. |
| [Ringwraith](docs/heroes.md#ringwraith) | Dread | **New hero.** Terrify foes and raise the slain as thralls. Own skills and gear. |
| [Saruman](docs/heroes.md#saruman) | Empowered spells | **New hero.** Cast three spells, then your next damaging spell deals +50% damage. |

## Decompilation

Alongside the mod, this fork continues RagingCactus's decompilation: rewriting the original game as readable C that
compiles back to the exact same bytes. Every recovered function is checked against the original ROM, and the
rebuilt ROM is byte-identical to the original (SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`).

| | Upstream base | This fork (2026-10-25) |
| --- | --- | --- |
| C source files | 51 | 356 |
| Decompiled C functions | not counted | 711 (89,644 compiled bytes) |
| Mapped functions (named, with or without C) | 772 | 1,059 |
| Handwritten data symbols (`config/data.cfg`) | 356 | 487 |

What this fork decompiled, by system:

| System | Files | Highlights |
| --- | --- | --- |
| Mission scripts | 85 | 63 script opcode handlers and the mission file loaders |
| Actors | 41 | Allocation, movement, collision, death, stun, knockback, aggro, enemy spawners |
| Scenes | 38 | Shop, skill tree, new game, logo screens, victory |
| Sprites | 33 | OAM, affine matrices, palette and cell caches, animation binding |
| Graphics | 27 | BG layers, scrolling, rotation and zoom, color effects, palette fades |
| Skills, player, RNG, effects | 51 | Active skill ticks, HUD and stats, the gameplay RNG, particles and status effects |
| Multiplayer, EEPROM, text, sound, save and others | 30 | |

The game is **not** fully decompiled. Parts without C are still copied from your own ROM when building, and the
function map is incomplete, so there is no honest "percent done" yet.

- Status, file layout and how verification works: [docs/decompilation.md](docs/decompilation.md)
- Every C file and function, and what is still missing: [docs/source-inventory.md](docs/source-inventory.md)
- How to decompile a function yourself: [docs/pipeline.md](docs/pipeline.md) and [docs/workflow.md](docs/workflow.md)

Check it yourself with `make verify` (rebuilds the ROM and compares the hash) and `make coverage` (live counts).

## Community documentation

Start at the [documentation index](docs/README.md) for player, contributor and testing guides. To continue the work, start with [where the work stands](docs/status.md).

- [Heroes](docs/heroes.md): every hero's idea, mechanic and skills.
- [Mod options](docs/mod-options.md): every mod options page, all Twists and their values.
- [Engine changes](docs/engine-changes.md): hooks, memory and save layout, the skill system and the combat system.
- [Performance](docs/performance.md): measured slowdown improvements and how to reproduce the benchmarks.
- [Pipeline](docs/pipeline.md): from Ghidra to byte matching, patch building, testing and release.
- [Expansion changes](docs/expanded-changes.md): hero content, engine systems, tooling and known testing limits.
- [Decompilation status](docs/decompilation.md): what the counts mean and how byte verification works.
- [Source inventory](docs/source-inventory.md): every original-game C file, mapped function without C, reconstructed data region, mod patch and authoring tool.
- [Contributing](CONTRIBUTING.md): development checks, documentation updates and useful bug reports.

The inventory can be regenerated with `uv run python tools/documentation_inventory.py` without a ROM.

## Playing it

1. Get `ROTK-Expanded-experimental-v0.8.bps` from the [v0.8 experimental release](https://github.com/Lillypad-14/ROTK-Expanded-experimental/releases/tag/v0.8-experimental).
2. You need your own legally dumped EU/US ROM (SHA-1 `ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`).
3. Apply the patch with [RomPatcher.js](https://www.marcrobledo.com/RomPatcher.js/) and play in mGBA or similar.

No ROM or game art is included in this repository.

## Building and modding

Same as upstream: see [docs/setup.md](docs/setup.md), then `make hack` (ROM) or `make dist` (BPS).
`dist` also needs [Flips](https://github.com/Sir-Walrus/Flips/releases).

Guides for extending the mod:

- [hacks/README.md](hacks/README.md): hook kinds and how patches are wired.
- [docs/wip-docs/howto-heroes-skills.md](docs/wip-docs/howto-heroes-skills.md): adding hero skills and mechanics.
- [docs/wip-docs/hacking-recipes.md](docs/wip-docs/hacking-recipes.md): reverse-engineering recipes and traps.
- [docs/wip-docs/fx-catalog.md](docs/wip-docs/fx-catalog.md): the effect kit and what each stock effect looks like.
- [docs/wip-docs/skill-tree.md](docs/wip-docs/skill-tree.md): per-hero designs, as-built notes and test lists.

For maintained entry points, use the [documentation index](docs/README.md). `docs/wip-docs/` is the inherited raw working notebook: research notes, decomp candidates, compiler traces and logs. It is
unpolished and partly out of date. ROM fragments and ripped sprite sheets were removed from it. Regenerate the sheets
from your own ROM with `make carve` and the tools in `hacks/tools/`.

---

# Original project README (lotr-gba-re/rotk, by RagingCactus)

The text below is the upstream README, written by RagingCactus. "I" and "my" refer to them.

Reverse engineering, decompilation, and ROM hacking for the game _Lord of the Rings: The Return of the King_ on the Game Boy Advance.

Currently a decent chunk of code has been decompiled, but large parts are still missing.
However, the parts that have been decompiled build as an exact byte-for-byte match of the original ROM.
It's difficult to measure progress on this, as large parts of the ROM are assets, which I haven't looked too deep into.

The reverse-engineered names and structures can also be used for debugging the ROM in mGBA via GDB (`make debug`, see [docs/workflow.md](docs/workflow.md)).

If you are mainly interested in what my ROM hack changes or how to run it, please see [the ROM hack user documentation](./docs/romhack-user.md).

> [!warning]
> This is a hobby project I created with lots of LLM assistance.
> In places I care about (how the matched C looks, for example) I made a reasonable effort to not produce slop and did a lot of human cleanup.
>
> However, other places do look a bit more sloppy.
> Some of the documentation and _especially_ the build scripts (make and python) are _very_ rough and are not up to my usual quality standards.
>
> That being said, I do believe this has potential and that the bad parts can be cleaned up eventually if this goes somewhere.
>
> This is also all work in progress, so take decompiled function and structure names with a grain of salt.
> If there is matching C, there is a reasonable chance of names being mostly correct.
> Otherwise, be careful and apply your best judgement.

If you encounter issues with the decomp or the ROM hack, feel free to open an issue or a discussion on this repository or visit the [**LOTR GBA Discord community**](https://discord.gg/yjHm8uf49e).

## Repository Contents

The repository contains reversed game code, tooling, and data tables (e.g. for item stats).
**The repository does not, will not, and must not contain art assets such as item art, sound effects, or music.**
If you want to rebuild the ROM or continue working on it, you must provide your own legally obtained ROM dump.

Directory overview:

- `src/c/`, `include/`, `config/`: Matched decompiled functions, reverse-engineered headers, and config files mapping names to ROM addresses.
- `carved/`: Committed data tables (affixes, loot, enemies, ...) carved from the ROM using `rotkit`. While these are machine-generated, they are committed for easier reference.
- `carved-local/`: Directory for art assets and asset metadata carved from the ROM into editable formats.
  **Gitignored**, see above for more information.
  Every clone re-carves them from its own ROM; `make clean` preserves them.
- `tools/rotkit/`: Python toolkit for data carving, original ROM building, working on byte-matching, and ROM hack building.
  Beware of slop.
- `make/`: Makefiles. Beware of slop.
- `hacks/`: modular ROM hack infrastructure and the patches themselves.
- `docs/`: setup, workflow, and conventions.

> [!warning]
> **NEVER include the binary ROM or original game art assets in pull requests or forks.**
>
> Art assets (item art, music, sounds, ...) must use the `carved-local` approach where every developer must provide their own ROM and use the tooling to re-carve them locally.
> These assets must never touch the committed repository.

## Quick Start

Drop a ROM dump as `rotk.gba` in the repo root.
Its SHA-1 must match `baserom.sha1`.

```sh
make check-tools    # toolchain, ROM checksum, Ghidra version
make verify         # rebuild the ROM from source; SHA must match the base
make hack           # build the ROM hack
make ghidra         # create a ghidra project based on the reverse-engineered code
```

See [docs/setup.md](docs/setup.md) for more information.

## ROM Hacks

`hacks/` composes self-contained patches into one patched ROM (`make hack`) and a distributable BPS patch (`make dist`).
Patches are written as armips assembly, as C bodies behind thin assembler hooks, or as typed edits of the carved data tables.
[hacks/README.md](hacks/README.md) covers the mechanics.

See the [ROM hack user documentation](./docs/romhack-user.md) for more information on which patches are already included.
In addition, there are a few demo patches, e.g. for data table edits.

## Detailed Documentation

- [Naming and coding conventions](./docs/conventions.md)
- [ROM hack user documentation](./docs/romhack-user.md)
- [Toolchain setup instructions](./docs/setup.md)
- [Workflow documentation](./docs/workflow.md)
- [ROM hacking developer README](./hacks/README.md) (Careful, currently still slop)
