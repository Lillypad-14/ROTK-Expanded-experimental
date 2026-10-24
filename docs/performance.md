# Performance changes

## OBJ tile search

`fast_obj_tiles` is enabled in the default patch selection.
It replaces the 140-byte ARM routine at `0x0800e17c`, which native startup copies to `0x03007690` using `bios_cpuSet`.
The replacement stays within that same buffer and adds no heap allocation or reserved RAM.
Its source is [fast_obj_tiles.asm](../hacks/patches/fast_obj_tiles/fast_obj_tiles.asm).
Cold-boot the ROM after changing this patch: emulator savestates retain the previous IWRAM code copy.

The search skips fully occupied 32-tile words and reduces work in mixed bytes.
It preserves the native byte traversal and its treatment of unaligned starting tile indices, rather than substituting a different first-fit policy.
Callers still receive the same first tile or `0xffff` failure result.
It does not change enemy updates, effect counts, damage or skill timing.

## Measured scope

These mGBA busy-fight comparisons were measured on 2026-10-23 using otherwise byte-identical arena ROMs.
All 96 differing bytes were inside the tile-search routine.
Sampling was disabled for timing measurements, with the same benchmark buffer reservation in each run.
Each encounter maintained a requested crowd of 16 enemies and repeatedly cast available abilities.

| Hero | Measured ticks | Original refreshes/tick | Optimized refreshes/tick | Lagging ticks, original / optimized |
| --- | ---: | ---: | ---: | ---: |
| Saruman | 1,200 | 6.817 | 6.661 | 1,196 / 1,194 |
| Sam | 600 | 2.307 | 2.272 | 183 / 162 |
| Aragorn | 600 | 2.227 | 2.227 | 129 / 128 |

The full-speed target is two screen refreshes per gameplay tick.
Lower values mean less slowdown.
Saruman's average tick duration improved about 2.3%, and Sam's about 1.5%.
Aragorn's rounded average was unchanged.
Average enemy counts and accepted cast counts matched between each pair.
These are specific stress tests, not a promise of the same gain in every encounter.
Heavy spell fights still slow down substantially.

## Reproduce the checks

With the original ROM and armips installed, compare both ARM routines directly:

```sh
uv run --with unicorn python hacks/tools/check_obj_tile_search.py
```

The checker assembles the production patch, verifies its 140-byte limit, and compares it with the original ROM in an ARM emulator.
It covers fragmented bitmaps, byte/word boundaries, contiguous free runs, valid starts through tile 1024, tile counts through 1024, random trailing memory, callee-saved registers, stack bounds, bitmap preservation, and ARM/Thumb returns.
Its default suite contains 23,849 cases with a fixed seed.
An extended run with `--cases 100000 --seed 104729` passed 103,849 cases on Unicorn's ARMv4T model.
Use `--cases` and `--seed` for additional random cases.

The baseline for the measurements above used the default patch selection with `fast_obj_tiles` disabled.
The optimized build used the same selection with that patch enabled.
Build each variant with `test_arena` selected and retain its matching ELF.
Use the benchmark's `--rom` and `--elf` options to select the pair explicitly:

```sh
uv run python hacks/tools/arena_bench.py --emulator /path/to/mGBA \
  --rom build/arena-variant.gba --elf build/arena-variant.elf \
  --hero 9 --crowd 16 --ticks 1200 --no-samples --output build/arena-runs/comparison
```

Each report records source and patched ROM hashes.
See [test arena](test-arena.md#busy-fight-benchmark) for sampling overhead and dropped-sample reporting.

## Sprite OAM submission

`fast_oam` replaces `sprite_pushOam` with a Thumb leaf routine in the same 80-byte ROM span.
It saves no registers on the stack and uses no additional RAM.
Successful submissions execute 24 instructions instead of 33.
Both shadow pages, the allocation counter, the full-buffer return and source-overlap behavior are preserved.

The production assembly passed 14,432 native differential cases on an ARMv4T emulator:

```sh
uv run --with unicorn python hacks/tools/check_oam_submit.py
```

These cover both shadow buffers, all byte-sized indices and allocation counts, upper argument bits, overlapping source entries, preserved registers, stack bounds and ARM/Thumb returns.
An extended run with `--cases 100000 --seed 104729` passed 104,432 cases.
An otherwise identical 1,200-tick Saruman/16-enemy benchmark changed from 6.661 to 6.657 refreshes per tick.
Lagging ticks remained 1,194, with the same 87 casts and 14.76 average enemies.
Only 73 bytes differed, all within the writer's ROM span.
This is a small local optimization, not a substantial heavy-fight improvement.

Both the tile-search build and the build with `fast_oam` enabled completed the 13-row sweep for all ten heroes without hangs or startup failures.
The only reported problems were the already documented second-cast cooldown warnings for Frodo row 3, Gandalf row 0, and Eowyn rows 1 and 2.

## Experimental frame composer

`fast_sprite` is optional and disabled by default.
It recompiles the recovered frame composer as Thumb code in the existing modcode ROM region and skips dimension lookups when the piece cannot use them.
It adds no heap allocations or mutable globals and does not relocate code into IWRAM.

The integrated replacement passed 100,640 native differential cases on ARMv4T, including affine modes, flips, draw flags, clipping, scale changes, attachment-record offsets, nonzero frame indices, both shadow buffers, full allocation counters, preserved registers and ARM/Thumb returns.
All ten heroes completed the 13-row arena sweep without hangs or startup failures, with only the documented cooldown warnings.

These isolated comparisons differ in 15 bytes, all in the 16-byte function-entry hook.
The replacement code is present in both ROMs and unreachable in the baseline.
Sampling was disabled, with a requested crowd of 16 enemies:

| Hero | Ticks | Refreshes/tick, original / experimental | Lagging ticks | Worst tick refreshes |
| --- | ---: | ---: | ---: | ---: |
| Saruman | 1,200 | 6.657 / 6.617 | 1,194 / 1,194 | 29 / 31 |
| Sam | 600 | 2.262 / 2.255 | 156 / 152 | 4 / 4 |
| Aragorn | 600 | 2.220 / 2.212 | 123 / 119 | 4 / 4 |

Accepted cast counts and average enemy counts matched within each pair.
The average improvement is about 0.3% to 0.6%, but Saruman's worst tick regressed.
That tradeoff is why this patch remains outside the default selection.

To test it, add `fast_sprite` to the selected patches:

```sh
make hack HACKS="@cfg test_arena fast_sprite" HACK_OUT=build/fast-frame-arena.gba
cp build/hack.elf build/fast-frame-arena.elf
uv run --with unicorn python hacks/tools/check_sprite_compose.py \
  --optimized build/fast-frame-arena.gba --write-baseline build/native-frame-arena.gba \
  --cases 100000 --seed 104729
```

The checker verifies the base ROM checksum and restores the original 884-byte routine into a copy of the optimized ROM to construct the baseline.
Use the same saved ELF with both ROMs when running `arena_bench.py`.

## Experimental audio

The native sound mixer runs from IWRAM and is the largest cost that grows with enemy count.
The `experimental` patch adds a Performance mod options page. Every toggle is off by default, so vanilla behavior is unchanged.
The three audio toggles are described here, and [frame pacing](#frame-pacing), [camera ease](#camera-ease), [fast RAM](#fast-ram) and the [effect cap](#effect-cap) follow.
Toggling rewrites the mixer's IWRAM code live with interrupts masked, and restores the original words from the ROM image when turned off.
Nothing is patched unless the IWRAM image matches the known mixer.

- **Fast Mixer** skips the blend of neighboring samples (nearest neighbor), which sounds rougher.
- **Voice Cap** limits sound effect voices to 16, 12, 8 or 6 of the 32, so extra sounds are dropped.
- **Mute Audio** skips every voice in the mixer, so the game is silent. Voices still advance, so sounds end on time.

Arena benchmark, 16 enemies, 600 ticks, refreshes per tick (2.0 is full speed):

| Setting | Hero 2 | Saruman |
| --- | ---: | ---: |
| Vanilla | 2.220 | 5.218 |
| Fast Mixer | 2.083 | 4.653 |
| Voice Cap 12 | 2.103 | 4.605 |
| Both, cap 12 | 2.032 | 4.225 |
| Voice Cap 8 | 2.040 | 4.232 |
| Both, cap 8 | 2.015 | 3.977 |
| Mute Audio | 2.003 | 3.562 |

In Saruman's fight the time per tick drops by 11% with Fast Mixer (5.218 to 4.653), 12% with Voice Cap 12 (4.605) and 19% with Voice Cap 8 (4.232).
Lighter fights gain less, for example 6% for Fast Mixer with hero 2.

Saruman's worst tick drops from 25 to 7 refreshes with both on.
Most of his remaining lag is not audio.

## Frame pacing

The stock `rotk_main` loop always waits for at least one vblank after a game tick, so a tick that needs slightly more than two vblanks idles until the third.
The **Frame Pacing** toggle replaces that loop and starts a late tick at once.
Its logic overlaps the wait for the previous frame's commit, and hooks on `sprite_commitGfx`, `sprite_composeOam` and `sprite_endFrame` hold the new frame's sprite writes until that commit.
Freed sprite tiles only return to the allocator in `system_endFrame`, so the overlap does not reuse tiles the displayed frame still needs.
With the toggle off, the loop performs the stock wait sequence and the hooks return at once.

Arena benchmark, 16 enemies, 600 ticks:

| Hero | Off | On | Lagging ticks, off / on | Worst tick, off / on |
| --- | ---: | ---: | ---: | ---: |
| Hero 2 | 2.228 | 2.180 | 128 / 108 | 4 / 4 |
| Saruman | 5.288 | 5.152 | 596 / 591 | 16 / 29 |

Casts and enemy counts matched within each pair.
The hooks cost about 0.4% with the toggle off.
Saruman's worst tick is longer with the toggle on, and the cause is not known.
The camera update at the start of an overlapped tick may also show the background one tick ahead of the sprites on lagging frames.

## Camera ease

The **Camera Ease** toggle (Off, Light, Strong) replaces the player position getter at `0x08014004`.
When `gfx_updatePlayerCameraScroll` calls it, the returned position closes half (Light) or a quarter (Strong) of the remaining gap each tick, so the camera's clamping, tile streaming and sprite offsets all use the eased position.
Moves larger than 48 pixels snap at once.
Other callers get the unchanged position, and arena casts, enemy counts and timing matched with every setting.

## Fast RAM

The **Fast RAM** toggle sets the undocumented memory control register at `0x04000800` to one work RAM wait state instead of two.
It works on GBA and GBA SP hardware but not on every model, so it stays off by default.
The mixer, collision and tile search already run from IWRAM, so the gain is small:

| Hero | Off | On |
| --- | ---: | ---: |
| Hero 2 | 2.228 | 2.212 |
| Saruman | 5.288 | 5.185 |

Casts and enemy counts matched within each pair.

## Effect cap

The **Effect Cap** toggle (Off, 24, 16, 8) is on the Performance 2 page.
Before `fx_kit` spawns an effect, it counts the live actors in `ACTOR_STATE_FX_ATTACHED` and refuses the request once the count reaches `hackLib_fxCap`.
Kit callers already treat a refused effect as normal, so damage and timing are unaffected.
The stock game's own `fx_spawnOnActor` calls are not capped, because many of its call sites write into the spawned actor without checking for failure.

| Setting | Ringwraith | Saruman |
| --- | ---: | ---: |
| Off | 5.645 | 5.288 |
| 16 | 5.068 | 5.180 |
| 8 | 4.827 | 4.957 |

Casts and enemy counts matched across all settings.
Saruman gains less because more of his actors are projectiles and hitboxes, which the cap does not touch.

## Actor composition regression checks

The main actor composer can be compared against the original ROM with:

```sh
uv run --with unicorn python hacks/tools/check_sprite_oam_compose.py \
  --optimized build/rotk_hack.gba --cases 10000 --seed 104729
```

The current default ROM passed 11,008 cases, including 1,008 fixed combinations.
The checker covers inactive actors, missing tiles, frozen frame pieces, attachment order, previous-frame wrapping, cached sprite owners, sprite strips, affine modes, both shadow buffers and full allocation counters.
It compares return values, actor and shadow-buffer memory, camera calls, preserved registers, stack bounds and ARM/Thumb returns.
Camera position is supplied by a stub that clobbers caller-saved registers.
Frame composition and OAM submission execute from each tested ROM.
These checks exercise synthetic actor states and do not measure frame timing or replace gameplay testing.

For decompilation work, `--candidate path/to/candidate.c` compiles a draft with the period compiler and tests it in a temporary ROM image.
This mode requires the normal repository toolchain and does not change the playable ROM.
Behavioral agreement complements the required byte match before integrating original-game C.

## Upload-cost ceiling experiment

On 2026-10-24, reference arena builds were compared with both actor and attachment tile uploads bypassed.
This is an optimistic upload-cost experiment, not an implemented graphics cache or a playable graphics setting.
The native upload entry points at `0x08049b34` and `0x08049bac` were replaced with Thumb `bx lr` instructions in a separate test image.
Only two ROM bytes changed.
Tile allocation, actor updates, frame selection and OAM composition remained active.
Both runs cold-booted with sampling disabled and a requested crowd of 16 enemies.

| Hero | Ticks | Refreshes/tick, normal / bypassed | Average reduction | Lagging ticks, normal / bypassed | Worst tick refreshes, normal / bypassed |
| --- | ---: | ---: | ---: | ---: | ---: |
| Saruman | 1,200 | 6.657 / 6.291 | 5.5% | 1,194 / 1,178 | 29 / 33 |
| Sam | 600 | 2.262 / 2.040 | 9.8% | 156 / 24 | 4 / 3 |
| Aragorn | 600 | 2.220 / 2.093 | 5.7% | 123 / 54 | 4 / 4 |

Enemy counts and accepted casts matched in each pair: Saruman 14.76 enemies and 87 casts, Sam 16.00 and 500, Aragorn 15.94 and 544.
Saruman's worst tick increased despite the average improvement.
A real cache would retain necessary uploads and add lookup and invalidation work, so these numbers do not predict its speedup.
The original engine already skips some unchanged frames and shares tiles through the sprite-cell cache.
The next useful measurement is how many remaining uploads can reuse still-valid tile contents, including allocation reuse and overlapping VRAM writes.

The baseline SHA-256 was `71416ff63cc8871114c5deb0b1687c67997d829b6661134b0fca1a3818c34f33`.
The bypass image SHA-256 was `9b9edcfb4b85717ee8c9f020ee4a754f7a72c06ed391ee51b57a7bbd8170b6d8`.
The playable ROM was not replaced by this experiment.

## Further work

The byte-matched actor dispatcher and graphics-commit routine are separate groundwork for further optimization.
`sprite_composeFrame` is byte-matched in C (884 bytes).
`sprite_composeOam` still needs a matched C implementation before that relocation work proceeds.
The tile-search optimization does not relocate either routine.
Profiles also include substantial time in code already running from IWRAM, so relocation alone will not eliminate every source of slowdown.
