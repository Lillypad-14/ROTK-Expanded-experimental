# Contributing

Start with the [community documentation](docs/README.md) and choose original-game decompilation, expansion content, tooling, documentation or playtesting. Credit the original project and distinguish recovered original-game behavior from new mod behavior.

## Before changing code

Read [conventions](docs/conventions.md), [setup](docs/setup.md) and [workflow](docs/workflow.md). Native reverse engineering starts in Ghidra. Persistent symbols belong in config files and headers rather than annotations in a generated Ghidra project.

Edit handwritten sources under `src/`, `include/`, `config/` or `hacks/`. Regenerate `carved/` through its carvers rather than editing it by hand. Keep original ROMs and extracted art/audio out of submissions. Each developer supplies their own ROM and regenerates `carved-local/`.

New abilities should use the [shared APIs](docs/ability-support.md), [graphics pipeline](docs/skill-effects.md) and [delivery pipeline](docs/skill-deliveries.md). Document prerequisites, level scaling, targeting, failure behavior, cleanup and save implications when they affect contributors or players.

## Checks and documentation

- Original-game C: byte-match the changed region, register it in `config/split.cfg`, then run `make verify` and `make check`.
- Mod changes: run `make check-mod` and relevant focused checks. Exercise normal gameplay in addition to host tests when behavior depends on native services.
- Graphics: regenerate previews and inspect them, then test effects in-game for anchors, palettes, lifetimes and allocation pressure.
- Inventory changes: run `uv run python tools/documentation_inventory.py` and include the resulting documentation diff.
- Describe commands actually run, their outcomes and any skipped checks. An old validation log does not verify a new change.

Keep public guides factual and link to authoritative code. Put speculative notes and compiler probes in gitignored `local/wip-docs/`. Update [the documentation index](docs/README.md) when adding a guide.

## Reporting bugs

Include the build or commit, selected patches if custom, emulator/hardware and version, hero, skill and upgrade ranks, equipment bonuses, map, and reproducible steps. Say whether the issue affects targeting, damage, status, input, visuals or saving. A short recording or screenshot helps with graphical issues.

Provide a minimal sequence from a known state and the expected versus observed result. Mention whether it also occurs on a fresh character or only an existing save. Never attach a ROM or extracted original game assets.

## Pull requests

Explain the concrete behavior change and link relevant source or documentation. Separate byte-matched recovery from intentional gameplay changes. Include validation evidence and known limitations, especially untested native or co-op behavior. Publish BPS patches for playable releases rather than game ROMs.
