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

## Debugger interface

Load `build/test_arena.elf` in ARM GDB and connect to mGBA's GDB server.
`g_TestArena` points to a versioned `TestArenaControl` with magic `0x41524e41`.
`ready` is 0 during boot, 1 when usable, and 2 when practice-lane or initial fixture setup failed.
`testArena_checkpoint` is a breakpoint at the end of each arena tick, after telemetry is updated.

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
