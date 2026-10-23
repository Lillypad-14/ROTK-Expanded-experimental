# Pipeline

How work on this repository is done, from reverse engineering a stock function to shipping a BPS. Toolchain setup is
in [setup](setup.md), and the upstream build loop is in [workflow](workflow.md). This page shows how the pieces fit
together.

```
 Ghidra (read-only) ──> config/*.cfg + include/*.h ──> src/c/<Name>.c ──> rotkit match ──> split.cfg ──> make verify
        │                        (names, types)          (decomp C)       (byte match)                (SHA-1 = base ROM)
        │
        └──────────────> hacks/patches/<patch>/ ──> make hack ──> emulator test ──> make dist ──> dist/hack.bps
                           (hooks + mod C)          (versioned ROM copy)  (gdb, VRAM dumps)
```

## 1. Reverse engineering

1. Open the function in **Ghidra** through the GhidrAssist MCP server, or its command-line client. Read the
   decompilation and disassembly there and find cross-references.
2. **Never rename in Ghidra.** The Ghidra project is regenerated from the text stores (`make ghidra`), so names only
   last when they are written to:
   - `config/functions.cfg` (address, mode and name of each function)
   - `config/data.cfg` (data symbols)
   - `include/*.h` (structs, enums and prototypes)
3. Run `make check-stores` to lint the stores, and `make symbols` to regenerate the generated symbol files.

Notes that are not confirmed yet go in gitignored `local/wip-docs/`. A scrubbed copy of the long-lived notebooks
is published in [docs/wip-docs](wip-docs/).

## 2. Decompiling and byte matching

1. Write the function in `src/c/<Name>.c`, documented on its definition with `@romaddress`.
2. Match it against the original ROM with the original compiler (agbcc):

   ```sh
   uv run rotkit match <Name> src/c/<Name>.c --full
   ```

   Iterate until it prints `MATCH`. The [byte-matching skill](../.agents/skills/byte-matching/SKILL.md) lists the
   agbcc code generation quirks that decide matches, such as constant folding, prologue order and branch layout.
3. Add the region to `config/split.cfg`.
4. `make verify` rebuilds the whole ROM from source, and its SHA-1 must equal the base ROM
   (`ec3d126d37ea6e2ad7030d8a99cf87c6e46c03fd`). `make check` runs this plus the tests, store lint and
   `-DBUGFIX` builds.

See [decompilation status](decompilation.md) for what has been matched so far.

## 3. Writing a patch

Patches live in `hacks/patches/<name>/` and are selected by `hacks/patches.cfg`. They are compiled with ARM GCC, not
agbcc, and linked into the expansion area. The kinds of hook are in [hacks/README.md](../hacks/README.md), and what
Expanded hooks is in [engine changes](engine-changes.md).

Rules that keep patches safe:

- Prefer a whole-function `hookReplace` that calls the original, over rewriting instructions in place.
- Every `.org` edit carries a `; was:` comment with the stock instructions.
- Every table repoint checks the stock value with `readu32`, so a wrong address fails the build.
- Mod RAM lives in `MODCODE_BSS`, and persistent data goes in the extended save regions.
- Shared code goes in `hacks/lib/`, so heroes reuse targeting, timed effects, combat events and HUD bars.

## 4. Content and generated data

Hero tables, cast timings, skill deliveries and effects are written as JSON or PNG and turned into C by generators.
Edit the source, never the generated file.

| Source | Generator | Make target | Guide |
| --- | --- | --- | --- |
| `hacks/content/heroes.json` | `hacks/tools/hero_registry.py` | `hack-heroes` | [hero authoring](wip-docs/howto-heroes-skills.md) |
| `hacks/content/fx/`, `skill_fx.json` | `hacks/tools/skill_fx.py`, `fx_sprite.py`, `fx_palette.py` | `hack-fx`, `check-fx` | [skill effects](skill-effects.md) |
| `hacks/content/cast_timelines.json` | `hacks/tools/cast_timelines.py` | `hack-fx` | [skill deliveries](skill-deliveries.md) |
| `hacks/content/skill_deliveries.json` | `hacks/tools/skill_deliveries.py` | `hack-fx` | [skill deliveries](skill-deliveries.md) |
| Effect drafts in the browser | `hacks/tools/effect_editor.py` | `effect-editor` | [Effect Studio](effect-editor.md) |
| Text overrides | text override generator | `hack-text-overrides` | [hacks/README.md](../hacks/README.md) |

### Skill icons

New skill icons start as art drawn on a mockup of the skill tree screen.

1. `skill_icon_art.py slice` cuts the 15 icons (9 passives, 6 actives) out of the mockup into 32x24 PNGs in the shared
   skill palette. The icon frame and corners always come from the stock frame, so only the circle interior is
   taken from the art, and the grey stone backdrop becomes transparent.
2. `skill_icon_art.py build` turns a folder of those PNGs into a C file of tile assets, such as
   `hacks/patches/skill_expansion/legolas_icons.c`.

```sh
uv run python hacks/tools/skill_icon_art.py slice MOCKUP.png OUT_DIR --names a,b,...
uv run python hacks/tools/skill_icon_art.py build PNG_DIR OUT.c --prefix LegolasIcon
```

### Skill text

`hacks/tools/skill_text_fit.py` wraps every skill description with the ROM font's glyph widths and reports any that
need more than the three lines the skill tree box has.

## 5. Building

```sh
make hack          # build/rotk_hack.gba from hacks/patches.cfg
make hack HACKS="@cfg twists"   # the default selection plus one patch
make test-arena    # direct-boot combat sandbox, ARENA_HERO=2 by default
make dist          # hack + a verified BPS in dist/hack.bps (needs Flips)
```

Test builds are copied to versioned files, such as `ROTK-Mod-Legolas-v6.gba`, so an older build can be compared
against a newer one when a regression shows up.

## 6. Testing

| Check | What it covers |
| --- | --- |
| `make check-hacks` | Every patch builds without a ROM. This is the pre-commit gate. |
| `make check-mod` | `make check`, the selected hack build, and the content and runtime regressions. |
| `make check-abilities` | Host tests of the shared ability support in `hacks/tests/`. |
| `make check-hero-moves` | Focused host checks of hero kits. See [hero move checks](hero-move-checks.md). |
| `make check-fx`, `make check-effect-editor` | Effect tooling and generated effect assets. |
| `make check-elemental-hook` | Runs the patched native elemental hook on ARM. |

Host tests compile the mod C on the PC against stand-ins for game services, such as `hacks/tests/combat_event.c`
and `aragorn.c`. They prove logic, not animations or real encounters.

### In-game testing

- **Debug pages** (`debug_cheats`): god mode, max level, infinite Spirit and similar toggles. The Debug Lab shows
  combat events live, Ability Bench casts any skill at any level, and delivery preview plays a skill's visuals.
- **Test arena** (`make test-arena`): boots straight into a fight with a chosen hero. See [test arena](test-arena.md).
- **mGBA with the gdb stub** (`mGBA -g <rom>`, symbols from `make debug`): breakpoints, memory reads and calls into
  game functions. The [gdb skill](../.agents/skills/gdb-scripts/SKILL.md) lists what works against the stub.

#### VRAM dumps

Graphics bugs are found by dumping video memory from the running game and comparing it to what the code expects.

1. Stop at a gameplay frame and dump VRAM:
   `dump binary memory vram.bin 0x06000000 0x06010000`.
2. Read the BG control registers to find each layer's char and screen block.
3. Decode the tile map and tiles to see which tile numbers each part of the screen uses.

This is how the herb count glitch was found. The compass draws its herb count digits in BG tiles `0x4d..0x4f`, which
looked free, and the HUD bars were drawing there too. The bars now use tiles `0x62..0x6a`. The HUD layout section of
[hacking recipes](wip-docs/hacking-recipes.md) records the measured layout.

## 7. Releasing

1. `make check-mod`, then play the changed heroes in an emulator.
2. `make dist` builds `dist/hack.bps` and verifies it against the base ROM.
3. Attach the BPS to a GitHub release. Never commit or attach a ROM, ROM fragments or game art.

## AI-assisted work

Much of this mod was written with AI coding agents (Claude Code and Codex) working under the rules in
[AGENTS.md](../AGENTS.md):

- Ghidra first, and names only in the stores.
- No hand edits to `carved/`.
- Terse comments, and no em-dashes.
- No git state changes without the developer's say-so.
- Agents do not write gdb scripts by themselves.

The [hacking recipes](wip-docs/hacking-recipes.md) and [hero authoring](wip-docs/howto-heroes-skills.md) notebooks are
written so that a smaller model can extend the mod safely. They record each recipe together with the traps found
while building it. Update them when you find a new trap.
