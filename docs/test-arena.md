# Direct-boot test arena

`make test-arena` creates `build/test_arena.gba` and its matching `build/test_arena.elf`.
The optional `test_arena` patch is excluded from the normal patch selection.
It boots directly into the Isengard campsite terrain with story groups and scripts disabled, a fresh Aragorn, ordinary starting gear, level-one available skills, and one stationary Combat Lab dummy.
There is no menu navigation, language prompt, save import, or migration step.
SRAM reads return blank bytes and SRAM writes are discarded in this build.

The arena finds a clear practice lane near the mission spawn and faces the player right.
It uses the existing map graphics and collision rather than a newly drawn map.
`ARENA_HERO=3 make test-arena` selects Gandalf instead.
Hero IDs follow the character registry, with Aragorn 2 and Gandalf 3.
Rebuild for a different hero.

## Automated scenario

Put the ARM toolchain on PATH, then run:

```powershell
uv run python hacks/tools/test_arena_run.py `
  --emulator "C:/path/to/mGBA.exe" `
  --fixture 0 --recipe 5 --level 1 --ticks 90 `
  --output build/arena-runs/venom-01
```

The output directory must be new.
The runner copies the ROM and matching ELF into it, cold-boots mGBA with its GDB server, resets the fixture, fires the chosen recipe, advances gameplay ticks, and writes `result.json`.
It also retains the exact GDB scenario and debugger/emulator logs.
The report includes the ROM SHA-256, fixture, recipe, level, elapsed ticks, player resources, outgoing hit/kill/damage totals, and target HP.
It terminates only the emulator process it launched.
Port 2345 must be free.
`--gdb` selects an ARM GDB executable when it is not on PATH.

Fixture 0 is a stationary high-HP dummy, 1 is three active ordinary archers, and 2 is one active fire archer.
Recipe IDs come from `skill_deliveries.json`: 5 is venom, 6 is flame, and 7 is slowing burst.
Omit `--recipe` for a boot/idle or incoming-attack scenario.
The current runner issues real delivery calls through a mailbox, rather than simulating attack button input.
Use normal emulator controls for manual skill, movement, menu, and artwork checks.

## Hero sweep

Sweep every hero:

```powershell
uv run python hacks/tools/arena_sweep_all.py `
  --emulator "C:/path/to/mGBA.exe" --output build/arena-runs/sweep-all-01
```

It builds the arena once, then runs each hero in its own unthrottled mGBA instance, `--jobs` (default 5) at a time.
No debugger is involved.
The runner patches `testArena_config` in a copy of the ROM: `hero`, `sweepTicks` (`--ticks`) and `sweepFixture` (`--fixture`, default 1).
With `sweepTicks` set, the arena drives itself through every active row.
Each row resets the fixture, casts, waits `sweepTicks`, clears while effects may be live, resets, casts again, waits, and clears.

Results go through the mGBA debug log registers at warning level, captured in each hero's `mgba.log`:
`ARENA_ROW` lists the values of `TestArenaSweepValue` in order, `ARENA_DONE` ends a run, and `ARENA_FAIL` means the arena did not start.
`ARENA_LAG` reports each tick longer than two vblanks with the row, step, vblanks, ticks since the last reset and actor count.
A player death reloads the mission, which shows up as a lag tick of about 46 vblanks, and sets `playerDowned` for the row.
The reset command restarts the hero systems the same way a new mission does, so hero timers do not carry over between casts.
Some stock skills still refuse the second cast because their native cooldown outlasts the default 150 ticks.
A longer `--ticks` run confirms them.
Each hero folder gets a `result.json`, and the run gets a `summary.json`.
A rejected cast is reported as not castable rather than failed.
`hung` is true when a hero hit `--timeout` without finishing.
`--heroes 3 7` limits the run and `--no-build` reuses `build/test_arena.gba`.

## Busy fight benchmark

```powershell
uv run python hacks/tools/arena_bench.py --emulator "C:/path/to/mGBA.exe" `
  --hero 2 --crowd 16 --ticks 1200 --output build/arena-runs/bench-01
```

It patches `benchTicks`, `benchCrowd` and `benchSample` into a copy of the arena ROM.
The arena keeps `--crowd` orc archers alive around the player, refills HP and Spirit, clears skill cooldowns, and casts whatever is castable every tick.
After a 60 tick warmup it measures `--ticks` ticks and prints `ARENA_BENCH` and one `ARENA_PROFILE` line per `HackLibProfileSection`.
`vblanksPerTick` is 2.0 at full speed.

With sampling on, Timer 2 interrupts about 1000 times a second through a wrapper on the BIOS interrupt vector, and each tick prints the interrupted program counters as `ARENA_PC` lines.
`result.json` lists the share of samples per region and per function, named from `config/functions.cfg` and the arena ELF.
The sampler itself shows up as `samplerFlush`, so compare lag with `--no-samples`.
Timer 2 is otherwise only used by EEPROM writes, which the arena does not make.

## Debugger interface

Load `build/test_arena.elf` in ARM GDB and connect to mGBA's GDB server.
`g_TestArena` points to a versioned `TestArenaControl` with magic `0x41524e41`.
`ready` is 0 during boot, 1 when usable, and 2 when practice-lane or initial fixture setup failed.
`testArena_checkpoint` is a breakpoint at the end of each arena tick, after telemetry is updated.
`testArena_acknowledgedPoint` runs only on ticks that acknowledge a request.
Breaking there with the wait command avoids a debugger stop on every tick, which makes long scenarios much faster.

Version 2 adds engine health telemetry, updated every arena tick:

- `lagTicks`: ticks that took more than two vblanks.
- `worstTickVblanks`: most vblanks between two arena ticks.
- `heapRoom`: bytes between the highest heap allocation since boot and the end of the heap.
- `allocFailures` and `invalidFrees`: heap allocator totals since boot.
- `bssGuardIntact`: 1 while the guard words below the mod `.bss` are untouched.
- `heroStateMisuse`: accesses to the shared hero state block for a hero the player is not.

Version 3 adds the automatic sweep progress fields `sweepRow`, `sweepStep` and `sweepValues`.

Write `argument`, `level`, `cosmetic`, and `command`, then publish a new `request` value last.
Advance until `acknowledged` equals the request.
`result` is 1 on success and 0 on rejection.

| Command | Value | Argument |
| --- | --- | --- |
| Reset fixture | 1 | Encounter 0..2 |
| Launch recipe | 2 | Delivery recipe ID, level 1..5 |
| Begin hero skill cast | 3 | Active skill row |
| Refill HP and Spirit | 4 | Ignored |
| Clear targets, owner effects, summons, and deliveries | 5 | Ignored |
| Wait | 6 | Gameplay ticks before acknowledgement |

Cast acknowledgement confirms the cast was accepted, not that a later hit landed.
The cast follows its normal animation or timeline.
Hero casts use the prepared skill levels, whereas recipe launches use the mailbox level.
Reset returns the player to the practice lane, clears test effects and delivery cooldowns, refills resources, clears combat counters, and recreates the selected targets.
It does not recreate every hero passive, animation, inventory, or native combat state.
Cold-boot another run for a fully fresh scenario.

Random reseeding uses a fixed seed in this build.
Repeated cold-boot scenarios with the same commands and timing can be compared.
Different actions and timing still consume random values differently.
The sandbox covers single-player combat and does not replace campaign, co-op, ordinary-input, or hardware playtesting.
