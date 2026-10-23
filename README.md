# ROTK Expanded (experimental)

An experimental content mod for _The Lord of the Rings: The Return of the King_ (GBA), built on top of the
[lotr-gba-re/rotk](https://github.com/lotr-gba-re/rotk) decompilation and ROM hack framework.

> [!important]
> **All credit for the foundation goes to [RagingCactus](https://github.com/RagingCactus)** and the
> [lotr-gba-re/rotk](https://github.com/lotr-gba-re/rotk) project: the reverse engineering, the byte-matched
> decompilation, the `rotkit` toolkit, the modular hack build, and the original patches (extra save slots, mod
> options, loot QoL, chapter select, fast fades, and more).
> This repository adds experimental content on top of that work. Please support and contribute to the original project
> and join the [LOTR GBA Discord community](https://discord.gg/yjHm8uf49e).
> The upstream project has no license file, so its code remains RagingCactus's. This repository is shared for study
> and community use with credit, and will be changed or taken down on request.

## What this mod adds

- **Second skill page for every hero.** Twelve active rows and eighteen passives per hero, with three color-coded
  builds per page and a unique mechanic per hero:
  - Aragorn, Frodo (Burden meter on the HUD), Sam ("Samwise's Garden"), Smeagol ("Gollum's Grip"),
    Gimli ("The Tally"), Gandalf ("The Grey and the White", hold R to swap form), Legolas ("The Glint", timed
    Overdraw release for True Shots).
- **New playable heroes.** Ringwraith (hero 8, built from the Nazgul model, with its own gear) and Saruman
  ("The Prism").
- **Twists.** Playthrough modifier pages in the mod options.
- **Debug cheats and Unlocks pages** for testing.
- **Content toolkit** in `hacks/lib/`: FX kit (stock effects, recolors, sprite tints, custom PNG effects), combat
  events, targeting, movement, status effects, timed zones, summons, HUD meters and cast timelines.
- Engine and save changes needed for all of the above (skill system, skill tree screen, combat and per-frame hooks).

Most of this is **untested on hardware and only lightly tested in emulators**. Expect bugs and balance problems.

## Playing it

1. Get `hack.bps` from the [latest release](../../releases/latest).
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

`docs/wip-docs/` is the raw working notebook: research notes, decomp candidates, compiler traces and logs. It is
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
