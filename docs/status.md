# Where the Work Stands

A starting point for anyone picking up ROTK Expanded from this repository. Last updated 2026-10-23.

## Build and Check

1. Follow [setup.md](setup.md) (native Windows works without WSL), copy your ROM in as `rotk.gba`, run
   `uv sync`.
2. `make hack` writes `build/rotk_hack.gba`. A fresh clone builds the same ROM as the author's latest build.
3. After any gameplay change, run the hero sweep (about 1.5 minutes, needs mGBA):

   ```sh
   uv run python hacks/tools/arena_sweep_all.py --emulator "C:/Program Files/mGBA/mGBA.exe" \
     --output build/arena-runs/sweep-01
   ```

   Expected today: every row ok or "not castable", except Frodo row 3, Gandalf row 0 and Eowyn rows 1 and 2,
   whose stock cooldowns outlast the default 150 tick wait.
4. `uv run --group dev pytest hacks/tools -q` runs the host tests.
5. For performance work, `hacks/tools/arena_bench.py` measures lag in a busy fight and samples the CPU. See
   [test-arena.md](test-arena.md).

## Status by Area

| Area | State |
| --- | --- |
| Page 2 skill trees | Built for all ten heroes, with custom icon art. See [heroes.md](heroes.md). |
| Gandalf "The Grey and the White" | Built, waiting on a full playtest and balance pass. |
| Legolas "The Glint" | Built, waiting on a full playtest. |
| Ringwraith (hero 8) | Built with page 2 and wraith gear. Some thrall and ghost behaviors rest on unverified assumptions. |
| Saruman (hero 9) | Spell visual rework, ground shockwave and balance pass built. In-game check of every spell pending. |
| Twists, Debug, Unlocks pages | Done. See [mod-options.md](mod-options.md). |
| Engine health | Hero state ownership checks, heap Room telemetry and BSS guard words. The sweep reports all of them. |
| Performance | `fast_obj_tiles` improves native tile-search work within its existing IWRAM buffer. Measured stress-test gains are modest and scene-dependent. See [performance](performance.md). |
| Decompilation | See [decomp-handoff.md](decomp-handoff.md) and [decompilation.md](decompilation.md). |

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
