# Where the Work Stands

A starting point for anyone picking up ROTK Expanded from this repository. Last updated 2026-10-28.

The v0.8 source adds co-op support for the mod pages, per-character Vanilla or Expanded kits and Versus (Friendly Fire and Duel). v0.7 added Eowyn's experimental Against the Dark kit. The older audit predates both and does not certify their behavior.

See [the latest bug audit](bug-audit.md) for the current fixes, map exclusions, and automated coverage.

## Build and Check

1. Follow [setup.md](setup.md) (native Windows works without WSL), copy your ROM in as `rotk.gba`, run
   `uv sync`.
2. `make hack` writes `build/rotk_hack.gba`. A fresh clone builds the same ROM as the author's latest build.
3. After any gameplay change, run the hero sweep (about 1.5 minutes, needs mGBA):

   ```sh
   uv run python hacks/tools/arena_sweep_all.py --emulator "C:/Program Files/mGBA/mGBA.exe" \
     --output build/arena-runs/sweep-01
   ```

   Some native buffs and cooldowns last longer than the default 150-tick wait, so a rejected second cast needs investigation before calling it a gameplay failure.
   In the latest 360-tick sweep, Eowyn row 2 needed a 900-tick follow-up, which passed.
   Empty rows and unmet equipment or target requirements are reported as "not castable". See [the audit](bug-audit.md).
4. `uv run --group dev pytest hacks/tools -q` runs the host tests.
5. For performance work, `hacks/tools/arena_bench.py` measures lag in a busy fight and samples the CPU. See
   [test-arena.md](test-arena.md).

## Status by Area

| Area | State |
| --- | --- |
| Page 2 skill trees | Built for all ten heroes. Eowyn's new kit uses stock icons and remains experimental. See [heroes.md](heroes.md). |
| Gandalf "The Grey and the White" | Built, waiting on a full playtest and balance pass. |
| Legolas "The Glint" | Built, waiting on a full playtest. |
| Ringwraith (hero 8) | Built with page 2 and wraith gear. Some thrall and ghost behaviors rest on unverified assumptions. |
| Saruman (hero 9) | Reworked spells, empowerment and balance changes. Host and automated emulator checks passed; full manual visual and balance testing remains. |
| Gauntlet | v0.6: 94 boons, gear buy/sell shop, evolution goals and 28 available maps. Both Minas Tirith maps are excluded for corrupt terrain. See [Gauntlet](gauntlet.md). |
| Twists, Debug, Unlocks pages | Done. See [mod-options.md](mod-options.md). |
| Engine health | Hero state ownership checks, heap Room telemetry and BSS guard words. The sweep reports all of them. |
| Performance | `fast_obj_tiles` improves native tile-search work within its existing IWRAM buffer. Measured stress-test gains are modest and scene-dependent. See [performance](performance.md). |
| Decompilation | 711 C functions in 356 source regions; 1,059 mapped functions. See [current decompilation status](decompilation.md) and the [generated inventory](source-inventory.md). |

## Next Steps

- Playtest Gandalf, Legolas and Saruman page 2 against the test lists in `docs/wip-docs/skill-tree.md`.
- The actor dispatcher and `sprite_commitGfx` are byte-matched.
  The frame-composition helper is matched (884 bytes). Finish `sprite_composeOam`, verify safe IWRAM placement, then benchmark the relocated code.
  See [performance groundwork](decompilation.md#actor-and-sprite-performance-groundwork).
- Keep [heroes.md](heroes.md) and the README hero table in step when a hero's mechanic changes.

## Notes and Recipes

`docs/wip-docs/` holds the working notes, with personal paths scrubbed. The most useful ones:

- `howto-heroes-skills.md`: adding heroes, skills, passives and icons.
- `hacking-recipes.md`: patching recipes and traps, including the mod RAM budget.
- `skill-tree.md`: per-hero design, as-built notes and test lists.
- `fx-catalog.md`: the skill visual kit.
- `HANDOFF-ringwraith-page2-and-gear.md`: Ringwraith status and checklist.

Agent instructions (`AGENTS.md`, `CLAUDE.md`) refer to these notes under `local/wip-docs/`, which is
gitignored. Copy them there to work with an AI assistant:

```sh
mkdir -p local && cp -r docs/wip-docs local/wip-docs
```

Never commit stock game art. Graphics derived from the ROM are generated at build time into `build/`.
